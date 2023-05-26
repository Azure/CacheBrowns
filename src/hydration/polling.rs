use crate::source_of_record::SourceOfRecord;
use crate::store::KeyTrackingStore;

struct PollingCacheHydrator<Key, Value> {
    store: Box<dyn KeyTrackingStore<Key, Value>>,
    data_source: Box<dyn SourceOfRecord<Key, Value>>
}
