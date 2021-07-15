# Library Proposal & Design Review

## Problem statement

We've all heard that there are only 2 hard problems in computer science: naming thing, cache invalidation, and
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

Why is cache invalidation so hard?

## Corrective principles

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

### Degrees of freedom

### Pre-canned implementations

## Examples
