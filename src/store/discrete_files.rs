use std::collections::HashMap;
use std::fs::File;
use std::hash::Hash;
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::path::{Path, PathBuf};
use crate::store::CacheStoreStrategy;
use serde::{Serialize, Deserialize, Serializer, Deserializer};
use uuid::Uuid;

fn get<Serde, T>(path: PathBuf) -> Option<T>
    where T: Serialize + for<'a> Deserialize<'a>,
          Serde : DiscreteFileSerializerDeserializer<T>
{
    if let Ok(file) = File::open(path) {
        return Some(Serde::deserialize(BufReader::new(file))?);
    }

    None
}

fn put<Serde, T>(path: PathBuf, value: T)
    where T: Serialize + for<'a> Deserialize<'a>,
          Serde : DiscreteFileSerializerDeserializer<T>
{
    if let Ok(file) = File::create(path) {
        Serde::serialize(value, BufWriter::new(file))
    }
}

fn delete(path: &PathBuf) -> bool {
    return std::fs::remove_file(path).is_ok();
}

fn flush(directory: PathBuf) {
    let _ = std::fs::remove_dir_all(directory);
}

fn get_or_create_index_entry<Key>(cache_directory: &PathBuf, index : &mut HashMap<Key, PathBuf>, key: Key) -> PathBuf
    where Key: Eq, Key: Hash
{
    index.entry(key).or_insert_with(|| -> PathBuf {
        let mut path = PathBuf::new();
        path.push(cache_directory.clone());
        path.push(Uuid::new_v4().hyphenated().to_string());
        path
    }).clone()
}

/// Before using, strongly consider using the volatile version. Do you really need this cache to
/// rehydrate without hitting the source of record? You are sacrificing reboot to clear corruption
/// and now must consider N vs N+1 schema issues when downgrading or upgrading your application.
pub struct DiscreteFileCacheStoreNonVolatile<Key, Serde, T>
    where Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
          T: Serialize + for<'a> Deserialize<'a>,
          Serde : DiscreteFileSerializerDeserializer<T>
{
    cache_directory: PathBuf,
    index: HashMap<Key, PathBuf>,
    phantom_serde: std::marker::PhantomData<Serde>,
    phantom_t: std::marker::PhantomData<T>
}

impl<Key, Serde, T> DiscreteFileCacheStoreNonVolatile<Key, Serde, T>
    where Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
          T: Serialize + for<'a> Deserialize<'a>,
          Serde : DiscreteFileSerializerDeserializer<T>
{
    pub fn new(cache_directory: PathBuf) -> Self {
        Self {
            cache_directory,
            index: HashMap::new(),
            phantom_serde: Default::default(),
            phantom_t: Default::default(),
        }
    }
}

impl<Key, Serde, Value> CacheStoreStrategy<Key, Value> for DiscreteFileCacheStoreNonVolatile<Key, Serde, Record<Key, Value>>
    where Key: Clone + Eq + Hash + Serialize + for<'a> Deserialize<'a>,
          Value: Serialize + for<'a> Deserialize<'a>,
          Serde : DiscreteFileSerializerDeserializer<Record<Key, Value>>
{
    fn get(&self, key: &Key) -> Option<Value> {
        if let Some(path) = self.index.get(key) {
            return Some(get::<Serde, Record<Key, Value>>(path.clone())?.value);
        }

        None
    }

    fn put(&mut self, key: &Key, value: Value) {
        let path = get_or_create_index_entry(&self.cache_directory, &mut self.index, (*key).clone());
        put::<Serde, Record<Key, Value>>(path, Record { key: (*key).clone(), value })
    }

    fn delete(&mut self, key: &Key) -> bool {
        if let Some(path) = self.index.get(key) {
            return delete(path);
        }

        false
    }

    fn flush(&mut self) {
        flush(self.cache_directory.clone())
    }
}

// pub struct DiscreteFileCacheStoreVolatile<Key, Value> {
//     store: DiscreteFileCacheStoreNonVolatile<Key, Value>,
//     /// Controls if the flush should run also run on destruction. Always runs on construction.
//     flush_on_destruction: bool
// }
//
// impl<Key, Value> DiscreteFileCacheStoreVolatile<Key, Value> {
//     fn new(cache_directory: PathBuf, flush_on_destruction: bool) -> Self {
//         let mut store = DiscreteFileCacheStoreVolatile {
//             store: DiscreteFileCacheStoreNonVolatile::new(cache_directory),
//             flush_on_destruction
//         };
//
//         store.flush();
//         store
//     }
// }
//
// impl<Key, Value> CacheStoreStrategy<Key, Value> for DiscreteFileCacheStoreVolatile<Key, Value> {
//     fn get(&self, key: &Key) -> Option<Value> {
//         self.store.get(key)
//     }
//
//     fn put(&mut self, key: &Key, value: Value) {
//         self.store.put(key, value)
//     }
//
//     fn delete(&mut self, key: &Key) -> bool {
//         self.store.delete(key)
//     }
//
//     fn flush(&mut self) {
//         self.store.flush()
//     }
// }
//
// impl<Key, Value> Drop for DiscreteFileCacheStoreVolatile<Key, Value> {
//     fn drop(&mut self) {
//         if self.flush_on_destruction {
//             self.flush()
//         }
//     }
// }

#[derive(Serialize, Deserialize)]
struct Record<Key, Value>
{
    pub key: Key,
    pub value: Value
}

/// Abstracts away the selection / creation of Serialize/Deserialize format
/// Buffered reads are used because if you are writing to disk either:
///     1. You are working with large data
///     2. You are just persisting, but everything is small and performance is fine anyway. By
///        nature of having selected discrete files you expect a fair amount of I/O sys-calls.
pub trait DiscreteFileSerializerDeserializer<T>
    where for<'a> T : Serialize + Deserialize<'a>
{
    fn serialize(value: T, buffered_writer: BufWriter<File>);

    fn deserialize(buffered_reader: BufReader<File>) -> Option<T>;
}

struct JsonDiscreteFileSerializerDeserializer {

}

struct FlexBuffersDiscreteFileSerializerDeserializer {

}
