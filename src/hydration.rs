pub mod polling;
pub mod pull;

#[derive(Debug, Eq, PartialEq)]
pub enum CacheLookupSuccess<Value> {
    /// Value was not present in the underlying store and had to be fetched from source of record.
    Miss(Value),

    /// Value was found in underlying store, but had to be refreshed from source of record.
    Refresh(Value),

    /// Stale value was found in underlying store, but fresh value could not be fetched from source of record.
    Stale(Value),

    /// Valid value found in underlying store.
    Hit(Value),
}

pub trait CacheHydrationStrategy<Key, Value> {
    fn get(&mut self, key: &Key) -> Option<CacheLookupSuccess<Value>>;

    fn flush(&mut self);

    fn stop_tracking(&mut self, key: &Key);
}

/// Enum representing store result
pub enum StoreResult {
    /// Valid value found in the store for the corresponding key.
    Valid,

    /// No value found in the store for the corresponding key.
    NotFound,

    /// Invalid value found in the store for the corresponding key.
    Invalid,
}

impl<Value> CacheLookupSuccess<Value> {
    pub fn new(store_result: StoreResult, hydrated: bool, value: Value) -> Self {
        match store_result {
            StoreResult::Invalid => {
                if hydrated {
                    CacheLookupSuccess::Refresh(value)
                } else {
                    CacheLookupSuccess::Stale(value)
                }
            }
            StoreResult::NotFound => CacheLookupSuccess::Miss(value),
            StoreResult::Valid => CacheLookupSuccess::Hit(value),
        }
    }
}
