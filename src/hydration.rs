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

impl<Value> CacheLookupSuccess<Value> {
    pub fn new(store_hit: bool, valid_entry: bool, hydrated: bool, value: Value) -> Self {
        if store_hit
        {
            if valid_entry
            {
                return CacheLookupSuccess::Hit(value);
            }
            else
            {
                if hydrated
                {
                    return CacheLookupSuccess::Refresh(value);
                }
                else
                {
                    return CacheLookupSuccess::Stale(value);
                }
            }
        }

        // TODO: Reconsider, with rust conversion it's possible to input failure data that gets here
        return CacheLookupSuccess::Miss(value);
    }
}
