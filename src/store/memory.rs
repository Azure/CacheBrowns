use crate::store::CacheStoreStrategy;
use std::collections::HashMap;

#[derive(Default)]
pub struct MemoryStore<Key, Value> {
    data: HashMap<Key, Value>,
}

impl<Key, Value> MemoryStore<Key, Value> {
    pub fn new() -> Self {
        MemoryStore {
            data: HashMap::new(),
        }
    }
}

pub type KeyIterator<'a, Key> = Box<dyn Iterator<Item = Key> + 'a>;

// TODO: Should everything be by ref return? Does that have concurrency issues?
impl<Key: Eq + Clone + std::hash::Hash, Value: Clone> CacheStoreStrategy<Key, Value>
    for MemoryStore<Key, Value>
{
    fn get(&self, key: &Key) -> Option<Value> {
        self.data.get(key).cloned()
    }

    fn peek(&self, key: &Key) -> Option<Value> {
        self.get(key)
    }

    fn put(&mut self, key: &Key, value: Value) {
        self.data.insert(key.clone(), value);
    }

    fn delete(&mut self, key: &Key) -> bool {
        self.data.remove(key).is_some()
    }

    fn flush(&mut self) {
        self.data.clear();
    }

    fn get_keys(&self) -> KeyIterator<Key> {
        Box::new(self.data.keys().cloned())
    }

    fn contains(&self, key: &Key) -> bool {
        self.data.contains_key(key)
    }
}
