# Library Proposal & Design Review

## Cache taxonomy

- Replacement: Algorithm that guides how (or if) entries are evicted when the capacity limit is reached.
- Hydration: The method by which data is retrieve and then stored
- Store: The underlying location + format of cached data
- Source of Record (SoR): The location the real data resides, used to hydrate the cache
- Cache Usage Patterns:
    - Cache-aside: Application managed cache. Application has connections to both the cache and the SoR. If entries are
      missing from the cache, it pulls the value from the SoR directly, then hydrates the cache with the returned value
    - Cache-as-SoR (managed caches): In these patterns the application operates on the cache directly, making it
      transparent
        - Read-through: Hit cache directly, if value is missing it is hydrated from SoR then returned
        - Write-through: Write to cache, make a synchronous write to SoR before returning control flow
        - Write-behind: Write to cache, queue a non-blocking write to the SoR

## Problem statement

We've all heard that there are only 2 hard problems in computer science: naming things, cache invalidation, and
off-by-one errors. This is an analysis on the way we contend with the challenges of cache invalidation, how they're
suboptimal, and how we can improve them.

Most applications rely upon some sort of external data store that locally stores the data to accelerate lookups. These
internal caches are application specific / custom and implemented in an ad-hoc manner. They're the source of much
consternation and cost:

1. Bugs. Cache invalidation is hard, and it's rare for an off the cuff implementation to be correct
1. Large scale duplication of effort (including redundant testing) as each service builds their own LRU and/or polling
   implementation, often times even multiple cache implementations per project.
1. Poor test coverage. Unit tests usually don't exist, and general test coverage usually isn't good enough (there's a
   lot of ways for the cache to be invalidated, are you covering all of them? Have you handled parallel use?)
1. Typically low code quality. The cache implementation is often tightly coupled to application specific details and or
   test coverage is black box end to end scenario testing (anecdotal experience, I don't have data to substantiate this)

## RCA

Why is cache invalidation so hard? While it is an inherently complex topic, we don't do ourselves favors as developers
with the litany of common unforced errors:

- Unwarranted optimization:
    - Lack of analysis showing that an optimization is necessary or that it's even faster in the real world
    - Misunderstanding of scale. If an implementation detail is slower but safer (say 1ms or less) you can almost always
      get away with the safer operation. Caching is meant to avoid events that are multiple orders of magnitude more
      expensive, the performance of the cache itself is largely irrelevant as a consequence.
    - Aggressive latency targets. E.g., persisting the cache to disk between reboots. Is it slower to rehydrate? Yes. is
      that small improvement worth the immense degree of additional complexity and increased work to mitigate live site
      worth it? Almost certainly not.
    - Aggressive freshness targets. Is it going to harm the system if data returned is marginally out date? Eventual
      consistency is a powerful tool for simplifying design.
- Failure to encapsulate
    - If consuming code can operate on the store directly or is aware of how/where the data is stored, you have a
      serious design smell on your hands.
- Violation of Single Responsibility Principle
    - Store and replacement are almost always coupled
    - Application specific concepts leak in (generics aren't used, error handling or other signaling, etc.)
- Manual management (cache writes)
    - Writing to the cache is where cache invalidation issues appear (yes they happen with reads, but when they do it's
      a failure to have performed a necessary write first)
    - Manual management provides opportunities to be faster or more custom, but how often is that really necessary? This
      is in the same vein as manual memory management vs managed languages. Manual absolutely has its place, but it is
      in specialized problems, not the general case.

Beyond these errors, caching often manifests itself as a shared resource in the multiple-readers multiple-writers
problem. In instances where the data is widely used among many features, the number of potential code paths explodes
exponentially, creating a breeding ground for temporal coupling, side effects, race conditions, and coherency issues.
This is all further compounded when parallelism is introduced.

## Corrective principles

To address this, we should strive to provide a generic read-through cache that full encapsulates not just the underlying
store, but also the writes, evictions, and staleness checks that all lead to cache invalidation. As we will see later on
in the examples, typical implementations don't actually require writes from the perspective of the application code. The
fact that the commonly do is merely an unnecessary application of the cache-aside pattern. If your application isn't
generating data bidirectionally with the SoR, then there's no need to support writes. Even in the case where writes or
event driven invalidations are needed this does not need to be handled by the reading application logic, it can be
injected into the managed cache (read: an instance of one-way-data-flow paradigm). A managed cache can satisfy these use
cases so long as it provides the following functionality.

### Requirements

1. Fully-Managed: The cache should expose only read operations, never write. If consumers never need to consider
   inserting, updating, or evicting from the cache then it can be truly transparent and decoupled from dependent logic.
   If we must write, or are able to, we can break strategy invariants and thus re-expose ourselves to the classes of
   cache invalidation bugs we're seeking to avoid here.
1. Programmable: In order for a cache to be fully-managed, cache writes must be fully generic. Retrieval logic and
   validation logic must be injectable, and in this sense the subset of a typical cache helper that truly is custom and
   application-specific can be programmed into the broader implementation.
1. Declarative: Common strategies should be made available, so that a consumer can simply declare what type of cache
   they need by chaining together the appropriate strategies. The interface can be made easier by naming common
   combinations.
1. Extensible: This differs from programmable in that for aspects of the cache that are unlikely to require custom
   implementations, it still must be possible to provide them for the obscure cases where they are needed. Additionally,
   this is the mechanism by which the strategies are decoupled.
1. Purgable Alternative: Legacy code with cache helpers will have instances where the cache needs to be invalidated
   directly by application code, because it is already coupled in this fashion. The fully-managed ideal described here
   is just that, an ideal. It must be possible for users to uptake this pattern piecemeal for incremental refactors or
   integration with legacy code (this complicates implementation and marginally harms performance, but not to a degree
   that is an issue for most use cases).
1. Trackable: It must be possible to easily see what happened and why when servicing a read request for performance
   tracking and debugging purposes. This does preclude certain types of minor optimizations, but is well worth it given
   that caching is bugprone and the performance data emitted can inform other system design tradeoffs that would net
   bigger gains than the added costs.

## Why not use an existing cache library?

If your code base has a cache helper in it, you're already tacitly supporting this notion ;) but since "we're already
doing it" is a bad justification, let's consider what factors push developers to create their own helpers rather than
use common implementations:

- Local-only applications of reliable managed caching strategies either:
    - Don't exist
    - Are extremely obscure
    - Do so only as implementation details of larger scale system clients
- Helpers are either:
    - Too specific
    - Designed for multi-node systems
    - Focus on abstracting away the cache store / replacement rather than providing a fully managed experience.

In other words, the elements all already exist, but aren't readily available for local in-memory stores.

Examples:

- MemCached: Intended for distributed systems, connects over network
- Redis : Managed solution, but highly distributed and operated as a distinct middleware service
- Cachelot : Very high performance memory store, only implements LRU algorithm
- Ehcache : Intended to offload database operations, java only
- .NET Framework: Not fully-managed, only handles expiration concepts (via timers or events); doesn't address issues for
  native code
- Misc : Templates and sample implementations of various replacement algorithms exist. These are what are typically
  copy-pasted for the cache helpers we find in most repos. They are not managed implementations

## Proposed implementation

A read-through cache that uses injected strategies to manage hydration and eviction transparently. Write-through can be
achieved via a custom strategy that when combined with the read-through interface creates a one-way-data-flow.

### Degrees of freedom & Pre-canned implementations

Each degree of freedom in a read-through cache represents a domain of strategies that could be injected.

1. Replacement
    1. NoReplacement
    1. LRU
1. Storage
    1. Memory
    2. Discrete Files
1. Hydration
    1. Polling
    1. Pull (read-through)
    1. InvokablePolling (invalid entries are fetched immediately)
    1. PubSub (delete => unsubscribe; get => subscribe; events => entry update)
1. Invalid entry response (enum)
    1. ReturnNotFound
    1. ReturnStaleData
1. Data source
    1. Just data provided
    1. Provide data and ability to check if still valid

### Interface

```
IManagedCache<Key, Value, whenInvalid>
    std::tuple<CacheLookupResult, Value> Get(Key)
    Flush()
    
ManagedCache<Key, Value, whenInvalid> : IManagedCache<Key, Value, whenInvalid>

PurgableCache<Key, Value, whenInvalid> : IManagedCache<Key, Value, whenInvalid>
    Evict(key)
    Replace(key) // evict & reload
    Refresh(key) // invalidate & reload
    Invalidate(key)

IRetrievable
ICacheDataSource : IRetrievable    
ICacheHydrationStrategy
ICacheReplacementStrategy    
```

### Requested feedback

Any and all feedback is helpful, but specific areas of interest include:

1. Premise validation: does the observation that the majority of custom helpers could be replaced with a read-through
   model hold?
1. Are any degrees of freedom missing?
1. Is there a commonly used strategy not listed here that would be advantageous to include
1. Is this useful for non-C++ projects?
1. Alternative ways to represent the degrees of freedom (i.e. a different interface hierarchy for strategy injection)

## Examples

TODO: Outline some scenarios
