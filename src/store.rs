pub mod discrete_files;
pub mod memory;
pub mod replacement;

pub type KeyIterator<'a, Key> = Box<dyn Iterator<Item = Key> + 'a>;

pub trait CacheStoreStrategy<Key, Value> {
    fn get(&mut self, key: &Key) -> Option<Value>;

    /// A platform read of a value. When replacement strategies are used (e.g. LRU) reads have side
    /// effects that update internal tracking. If hydration requires inspecting the current state,
    /// these reads will skew tracking. Peek allows you to inspect state without side effects, it
    /// signals to any layer that a platform read has occurred that should be ignored for usage
    /// tracking purposes.
    fn peek(&self, key: &Key) -> Option<Value>;

    fn put(&mut self, key: &Key, value: Value);

    // TODO: not sure if the bool is necessary, forget why it's there off hand
    fn delete(&mut self, key: &Key) -> bool;

    fn flush(&mut self);

    fn get_keys(&self) -> KeyIterator<Key>;

    fn contains(&self, key: &Key) -> bool;
}
