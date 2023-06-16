pub trait SourceOfRecord<Key, Value> {
    fn retrieve(&self, key: &Key) -> Option<Value>;

    /// Accepts current value if one exists in case it can be used for optimized load
    /// ex. If building an HTTP cache, and you receive a 304 response, replay the current value
    fn retrieve_with_hint(&self, key: &Key, _current_value: &Value) -> Option<Value> {
        //The default case is no current value based optimization, so provide pass-through implementation
        self.retrieve(key)
    }

    fn is_valid(&self, key: &Key, value: &Value) -> bool;
}

/// Operates http client with cache control headers respected
pub struct HttpDataSourceOfRecord {}

impl HttpDataSourceOfRecord {}

impl<Key, Value> SourceOfRecord<Key, Value> for HttpDataSourceOfRecord {
    fn retrieve(&self, key: &Key) -> Option<Value> {
        todo!()
    }

    fn retrieve_with_hint(&self, key: &Key, current_value: &Value) -> Option<Value> {
        todo!()
    }

    fn is_valid(&self, key: &Key, value: &Value) -> bool {
        todo!()
    }
}
