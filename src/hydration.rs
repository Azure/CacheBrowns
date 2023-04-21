pub mod pull;
pub mod polling;

pub enum CacheLookupSuccess<Value> {
    Miss(Value),
    Refresh(Value),
    Stale(Value),
    Hit(Value),
}

pub trait CacheHydrationStrategy<Key, Value> {
    fn get(&mut self, key: &Key) -> Option<CacheLookupSuccess<Value>>;

    fn flush(&mut self);
}
