use crate::store::CacheStoreStrategy;
use crate::store::KeyIterator;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::fs::File;
use std::hash::Hash;
use std::io::{BufReader, BufWriter};
use std::path::{Path, PathBuf};
use uuid::Uuid;

/// Before using, strongly consider using the volatile version. Do you really need this cache to
/// rehydrate without hitting the source of record? You are sacrificing reboot to clear corruption
/// and now must consider N vs N+1 schema issues when downgrading or upgrading your application.
pub struct DiscreteFileStoreNonVolatile<Key, Value, Serde>
where
    Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Value>,
{
    cache_directory: PathBuf,
    index: HashMap<Key, PathBuf>,
    phantom_serde: std::marker::PhantomData<Serde>,
    phantom_value: std::marker::PhantomData<Value>,
}

// TODO: refactor unnecessary clones (e.g. of path)

impl<Key, Value, Serde> DiscreteFileStoreNonVolatile<Key, Record<Key, Value>, Serde>
where
    Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Record<Key, Value>>,
{
    pub fn new(cache_directory: PathBuf) -> Self {
        let mut store = Self {
            cache_directory,
            index: HashMap::new(),
            phantom_serde: Default::default(),
            phantom_value: Default::default(),
        };
        Self::rehydrate_index(&mut store);

        store
    }

    fn rehydrate_index(store: &mut DiscreteFileStoreNonVolatile<Key, Record<Key, Value>, Serde>) {
        fs::create_dir_all(store.cache_directory.as_path()).unwrap();

        if let Ok(paths) = fs::read_dir(store.cache_directory.as_path()) {
            for path in paths.flatten() {
                if let Some(record) = get::<Serde, Record<Key, Value>>(path.path()) {
                    store.index.insert(record.key, path.path());
                }
            }
        }
    }
}

impl<Key, Value, Serde> CacheStoreStrategy<Key, Value>
    for DiscreteFileStoreNonVolatile<Key, Record<Key, Value>, Serde>
where
    Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Record<Key, Value>>,
{
    fn get(&self, key: &Key) -> Option<Value> {
        if let Some(path) = self.index.get(key) {
            return Some(get::<Serde, Record<Key, Value>>(path.clone())?.value);
        }

        None
    }

    fn peek(&self, key: &Key) -> Option<Value> {
        self.get(key)
    }

    fn put(&mut self, key: &Key, value: Value) {
        let path =
            get_or_create_index_entry(&self.cache_directory, &mut self.index, (*key).clone());
        put::<Serde, Record<Key, Value>>(
            path,
            Record {
                key: (*key).clone(),
                value,
            },
        )
    }

    fn delete(&mut self, key: &Key) -> bool {
        delete(&mut self.index, key)
    }

    fn flush(&mut self) {
        flush(self.cache_directory.clone())
    }

    //noinspection DuplicatedCode
    fn get_keys(&self) -> KeyIterator<Key> {
        Box::new(self.index.keys().cloned())
    }

    fn contains(&self, key: &Key) -> bool {
        self.index.contains_key(key)
    }
}

pub struct DiscreteFileStoreVolatile<Key, Value, Serde>
where
    Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Value>,
{
    cache_directory: PathBuf,
    index: HashMap<Key, PathBuf>,
    phantom_serde: std::marker::PhantomData<Serde>,
    phantom_value: std::marker::PhantomData<Value>,
}

impl<Key, Value, Serde> DiscreteFileStoreVolatile<Key, Value, Serde>
where
    Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Value>,
{
    pub fn new(cache_directory: PathBuf) -> Self {
        let store = Self {
            cache_directory,
            index: HashMap::new(),
            phantom_serde: Default::default(),
            phantom_value: Default::default(),
        };

        flush(store.cache_directory.clone());
        fs::create_dir_all(store.cache_directory.as_path()).unwrap();
        store
    }
}

impl<Key, Value, Serde> CacheStoreStrategy<Key, Value>
    for DiscreteFileStoreVolatile<Key, Value, Serde>
where
    Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Value>,
{
    fn get(&self, key: &Key) -> Option<Value> {
        if let Some(path) = self.index.get(key) {
            return get::<Serde, Value>(path.clone());
        }

        None
    }

    fn peek(&self, key: &Key) -> Option<Value> {
        self.get(key)
    }

    fn put(&mut self, key: &Key, value: Value) {
        let path =
            get_or_create_index_entry(&self.cache_directory, &mut self.index, (*key).clone());
        put::<Serde, Value>(path, value)
    }

    fn delete(&mut self, key: &Key) -> bool {
        delete(&mut self.index, key)
    }

    fn flush(&mut self) {
        flush(self.cache_directory.clone())
    }

    //noinspection DuplicatedCode
    fn get_keys(&self) -> KeyIterator<Key> {
        Box::new(self.index.keys().cloned())
    }

    fn contains(&self, key: &Key) -> bool {
        self.index.contains_key(key)
    }
}

fn get<Serde, Value>(path: PathBuf) -> Option<Value>
where
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Value>,
{
    if let Ok(file) = File::open(path) {
        return Serde::deserialize(BufReader::new(file));
    }

    None
}

fn put<Serde, Value>(path: PathBuf, value: Value)
where
    Value: Serialize + for<'a> Deserialize<'a>,
    Serde: DiscreteFileSerializerDeserializer<Value>,
{
    if let Ok(file) = File::create(path) {
        Serde::serialize(BufWriter::new(file), &value)
    }
}

fn delete<Key>(index: &mut HashMap<Key, PathBuf>, key: &Key) -> bool
where
    Key: Eq,
    Key: Hash,
{
    if let Some(path) = index.remove(key) {
        return fs::remove_file(path).is_ok();
    }

    false
}

fn flush(directory: PathBuf) {
    let _ = fs::remove_dir_all(directory);
}

fn get_or_create_index_entry<Key>(
    cache_directory: &Path,
    index: &mut HashMap<Key, PathBuf>,
    key: Key,
) -> PathBuf
where
    Key: Eq,
    Key: Hash,
{
    index
        .entry(key)
        .or_insert_with(|| -> PathBuf {
            let mut path = PathBuf::new();
            path.push(cache_directory);
            path.push(Uuid::new_v4().hyphenated().to_string());
            path
        })
        .clone()
}

#[derive(Serialize, Deserialize)]
struct Record<Key, Value> {
    pub key: Key,
    pub value: Value,
}

/// Abstracts away the selection / creation of Serialize/Deserialize format
/// Buffered reads are used because if you are writing to disk either:
///     1. You are working with large data
///     2. You are just persisting, but everything is small and performance is fine anyway. By
///        nature of having selected discrete files you expect a fair amount of I/O sys-calls.
pub trait DiscreteFileSerializerDeserializer<Value>
where
    for<'a> Value: Serialize + Deserialize<'a>,
{
    fn serialize(buffered_writer: BufWriter<File>, value: &Value);

    fn deserialize(buffered_reader: BufReader<File>) -> Option<Value>;
}

pub struct JsonDiscreteFileSerializerDeserializer<Value>
where
    for<'a> Value: Serialize + Deserialize<'a>,
{
    phantom: std::marker::PhantomData<Value>,
}

impl<Value> DiscreteFileSerializerDeserializer<Value>
    for JsonDiscreteFileSerializerDeserializer<Value>
where
    for<'a> Value: Serialize + Deserialize<'a>,
{
    fn serialize(buffered_writer: BufWriter<File>, value: &Value) {
        let _ = serde_json::to_writer(buffered_writer, &value);
    }

    fn deserialize(buffered_reader: BufReader<File>) -> Option<Value> {
        serde_json::from_reader(buffered_reader).ok()
    }
}

pub struct BincodeDiscreteFileSerializerDeserializer<Value>
where
    for<'a> Value: Serialize + Deserialize<'a>,
{
    phantom: std::marker::PhantomData<Value>,
}

impl<Value> DiscreteFileSerializerDeserializer<Value>
    for BincodeDiscreteFileSerializerDeserializer<Value>
where
    for<'a> Value: Serialize + Deserialize<'a>,
{
    fn serialize(buffered_writer: BufWriter<File>, value: &Value) {
        let _ = bincode::serialize_into(buffered_writer, &value);
    }

    fn deserialize(buffered_reader: BufReader<File>) -> Option<Value> {
        serde_json::from_reader(buffered_reader).ok()
    }
}

pub type DiscreteFileStoreNonVolatileBincode<Key, Value> =
    DiscreteFileStoreNonVolatile<Key, Value, BincodeDiscreteFileSerializerDeserializer<Value>>;
pub type DiscreteFileStoreVolatileBincode<Key, Value> =
    DiscreteFileStoreVolatile<Key, Value, BincodeDiscreteFileSerializerDeserializer<Value>>;
pub type DiscreteFileStoreNonVolatileJson<Key, Value> =
    DiscreteFileStoreNonVolatile<Key, Value, JsonDiscreteFileSerializerDeserializer<Value>>;
pub type DiscreteFileStoreVolatileJson<Key, Value> =
    DiscreteFileStoreVolatile<Key, Value, JsonDiscreteFileSerializerDeserializer<Value>>;
