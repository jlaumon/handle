#pragma once

// TODO add reserve
// TODO add doc

#include <type_traits> // std::is_integral/std::is_unsigned/std::forward

#ifdef HDL_USER_CONFIG
#include HDL_USER_CONFIG
#endif

#ifndef HDL_ASSERT
#include <assert.h>
#define HDL_ASSERT(condition, ...) assert(condition)
#endif

#ifndef HDL_DEQUE
#include <deque> // VC++ has a very bad deque implementation, prefer switching to eastl::deque or a custom FIFO that does not allocate each elements separately
#define HDL_DEQUE std::deque
#endif

#ifndef HDL_MUTEX
#include <mutex>
#define HDL_MUTEX std::mutex
#endif

namespace VirtualMemory
{
	size_t GetPageSize();
	/// Reserves a memory area of at least _size bytes. The memory needs to be committed before being used.
	void*  Reserve (size_t _size);
	/// Releases reserved memory. Also decommits any part that was committed.
	/// _address and _size must match the values returned by/passed to the Reserve call that reserved this memory area.
	void   Release (void* _address, size_t _size);
	/// Commits reserved memory. All newly allocated pages will contain zeros.
	/// All the pages containing at least one byte in the range _address, _address + _size will be committed.
	/// @returns Commit success.
	bool   Commit  (void* _address, size_t _size);
	/// Frees committed memory. 
	/// All the pages containing at least one byte in the range _address, _address + _size will be decommitted.
	void   Decommit(void* _address, size_t _size);
}

template <typename, typename, size_t> class HandlePool;

template <typename T, typename Tag = void,
	typename IntegerType = uint32_t,
	size_t MaxHandles = 64 * 1024
>
class Handle
{
public:
	typedef Handle<T, Tag, IntegerType, MaxHandles> this_type;
	typedef IntegerType                             integer_type; ///< The type of the (unsigned) integer inside the handle.
	typedef HandlePool<T, IntegerType, MaxHandles>  pool_type;

	static const integer_type kInvalid = ~0;

	template <class ... Args>
	static this_type Create  (Args&&... _args)   { return this_type(s_pool.create(std::forward<Args>(_args)...)); }
	static bool      Destroy (this_type _handle) { return s_pool.destroy(_handle); }
	static T*        Get     (this_type _handle) { return s_pool.get(_handle);}

	static size_t    Size    ()                  { return s_pool.size(); }
	static size_t    Capacity()                  { return s_pool.capacity(); }
	static size_t    MaxSize ()                  { return s_pool.max_size(); }

	static void      Reset   ();

	Handle()                              : m_intVal(kInvalid) {}
	Handle(const this_type& _handle)      : m_intVal(_handle.m_intVal) {}
	explicit Handle(integer_type _intVal) : m_intVal(_intVal) {}
	operator integer_type() const                  { return m_intVal; }
	this_type& operator=(integer_type _intVal)     { m_intVal = _intVal; return *this; }
	this_type& operator=(const this_type& _handle) { m_intVal = _handle.m_intVal; return *this; }

private:
	integer_type m_intVal;
	
	static pool_type s_pool;
};

template <typename T, typename Tag, typename IntegerType, size_t MaxHandles>
void Handle<T, Tag, IntegerType, MaxHandles>::Reset()
{
	// Call the destructor/constructor explicitely to destroy and recreate the pool
	s_pool.~HandlePool<T, IntegerType, MaxHandles>();
	new (&s_pool) HandlePool<T, IntegerType, MaxHandles>();
}

template <typename T, typename Tag, typename IntegerType, size_t MaxHandles>
HandlePool<T, IntegerType, MaxHandles> Handle<T, Tag, IntegerType, MaxHandles>::s_pool;

template <typename T, typename IntegerType, size_t MaxHandles>
class HandlePool
{
public:
	typedef HandlePool<T, IntegerType, MaxHandles> this_type;
	typedef IntegerType                            integer_type;
	static const integer_type kInvalid = ~0;

	HandlePool() = default;
	~HandlePool();
	
	HandlePool(const this_type&) = delete;
	this_type& operator= (this_type&) = delete;

	template <class ... Args>
	integer_type create  (Args&&... _args);
	bool         destroy (integer_type _handle);
	T*           get     (integer_type _handle);

	size_t       size    () const { return m_handleCount; }
	size_t       capacity() const { return MinSizeT(m_nodeBufferCapacityBytes / sizeof(Node), kMaxHandles); }
	size_t       max_size() const { return kMaxHandles; }

	static constexpr size_t MinSizeT(size_t _a, size_t _b) { return _a < _b ? _a : _b; } // Don't want to include <algorithm> just for std::min
	static constexpr size_t CeilLog2(size_t _x)            { return _x < 2 ? 1 : 1 + CeilLog2(_x >> 1); }

	static const size_t kMaxHandles     = MaxHandles;
	static const size_t kIndexNumBits   = CeilLog2(MaxHandles - 1);
	static const size_t kIndexMask      = ((size_t)1 << kIndexNumBits) - 1;
	static const size_t kVersionNumBits = MinSizeT(sizeof(IntegerType) * 8 - kIndexNumBits, sizeof(size_t) * 8 - 1); // The version is stored in a size_t minus 1 bit below.
	static const size_t kVersionMask    = ((size_t)1 << kVersionNumBits) - 1;

	static_assert(std::is_integral<IntegerType>::value && std::is_unsigned<IntegerType>::value, "IntegerType must be an unsigned integer type.");
	static_assert(kIndexNumBits < sizeof(IntegerType) * 8, "There are not enough bits in IntegerType to store both index and version.");
	
	// Choose the smallest index type that can fit the wanted number of bits.
	typedef typename std::conditional< kIndexNumBits <= 16, uint16_t,
		typename std::conditional<kIndexNumBits <= 32, uint32_t, uint64_t>::type >::type index_type;

	static index_type   GetIndex  (integer_type _handle);
	static size_t       GetVersion(integer_type _handle);
	static integer_type GetID     (index_type _index, size_t _version);

private:
	size_t getNodeBufferSize() const;

	struct Node
	{
		size_t m_allocated : 1;
		size_t m_version   : sizeof(size_t) * 8 - 1; // = 63 bits on 64 bits systems.
		T      m_value;
	};

	// The max value m_nodeBufferSizeBytes can take to keep its indexable with kIndexNumBits
	static const size_t kNodeBufferMaxSizeBytes = (1 << kIndexNumBits) * sizeof(Node);

	Node*                   m_nodeBuffer              = nullptr;
	size_t                  m_nodeBufferSizeBytes     = 0;
	size_t                  m_nodeBufferCapacityBytes = 0;
	size_t                  m_handleCount             = 0;
	HDL_DEQUE<index_type>   m_freeIndices;
	HDL_MUTEX               m_mutex;
};

template <typename T, typename IntegerType, size_t MaxHandles>
HandlePool<T, IntegerType, MaxHandles>::~HandlePool()
{
	// Destroy all the allocated nodes
	size_t nodeCount = getNodeBufferSize();
	for (size_t i = 0; i < nodeCount; ++i)
	{
		auto node = m_nodeBuffer + i;
		if (node->m_allocated)
			node->m_value.~T();
	}

	// Release the reserved memory
	if (m_nodeBuffer)
		VirtualMemory::Release(m_nodeBuffer, kMaxHandles * sizeof(Node));
}

template <typename T, typename IntegerType, size_t MaxHandles>
template <class ... Args>
IntegerType
HandlePool<T, IntegerType, MaxHandles>::create(Args&&... _args)
{
	m_mutex.lock();

	if (m_handleCount == kMaxHandles)
	{
		m_mutex.unlock();
		return kInvalid;
	}

	index_type index;

	// If there is enough space in the node buffer, add a node
	// Note: use the rest of the buffer before looking for free indices to delay the wrapping of the versions as much as possible
	if (m_nodeBufferSizeBytes < kNodeBufferMaxSizeBytes
		&& (m_nodeBufferSizeBytes + sizeof(Node)) <= m_nodeBufferCapacityBytes)
	{
		index = (index_type)getNodeBufferSize();
		m_nodeBufferSizeBytes += sizeof(Node);
	}
	// Otherwise look for free indices
	else if (!m_freeIndices.empty())
	{
		index = m_freeIndices.front();
		m_freeIndices.pop_front();
	}
	// Last option, grow the node buffer
	else
	{
		HDL_ASSERT(m_nodeBufferSizeBytes < kNodeBufferMaxSizeBytes); // At this point, either the freelist should not be empty, 
		                                                             // or we should have reached kMaxHandles and returned kInvalid

		index = (index_type)getNodeBufferSize();

		// Check how many pages we need to store at least one more node
		// FIXME! Not the best idea if nodes are very big, make alloc size customizable?
		size_t neededBytes = m_nodeBufferSizeBytes + sizeof(Node) - m_nodeBufferCapacityBytes;
		auto pageSize = VirtualMemory::GetPageSize();
		size_t nbPages = 1;
		if (neededBytes > pageSize)
			nbPages = 1 + neededBytes / pageSize;

		// Reserve the node buffer if it wasn't done yet
		if (!m_nodeBuffer)
			m_nodeBuffer = (Node*)VirtualMemory::Reserve(kMaxHandles * sizeof(Node));

		// Increase capacity by commiting more pages
		// Note: The memory allocated by VirtualMemory::Commit is zeroed, so m_version/m_allocated inside the nodes will automatically be initialized to 0
		if (!VirtualMemory::Commit((char*)m_nodeBuffer + m_nodeBufferCapacityBytes, nbPages * pageSize))
		{
			// Allocation failed. (Out of memory?)
			m_mutex.unlock();
			return kInvalid;
		}
		m_nodeBufferSizeBytes += sizeof(Node);
		m_nodeBufferCapacityBytes += nbPages * pageSize;
	}

	m_handleCount++;
	m_mutex.unlock();

	auto node = m_nodeBuffer + index;
	node->m_allocated = true;
	new (&node->m_value) T(std::forward<Args>(_args)...);

	return GetID(index, node->m_version);
}

template<typename T, typename IntegerType, size_t MaxHandles>
bool
HandlePool<T, IntegerType, MaxHandles>::destroy(integer_type _handle)
{
	if (_handle == kInvalid)
		return false;

	index_type index = GetIndex(_handle);
	size_t version = GetVersion(_handle);

	HDL_ASSERT(index < getNodeBufferSize());
	auto node = m_nodeBuffer + index;

	if (node->m_version != version)
		return false; // The handle was already destroyed.

	node->m_version++;
	// Force the version to wrap around to make sure it doesn't use more than VersionNumBits (otherwise the equality test would fail).
	node->m_version &= kVersionMask;

	// Special case for the last index: it cannot use the max version, otherwise the handle would be equal to kInvalid.
	// In this case, wrap around sooner.
	if (GetID(index, node->m_version) == kInvalid)
		node->m_version = 0;

	HDL_ASSERT(node->m_allocated);
	node->m_value.~T();
	node->m_allocated = false;

	m_mutex.lock();
	m_handleCount--;
	m_freeIndices.push_back(index);
	m_mutex.unlock();

	return true;
}

template<typename T, typename IntegerType, size_t MaxHandles>
T* 
HandlePool<T, IntegerType, MaxHandles>::get(integer_type _handle)
{
	if (_handle == kInvalid)
		return nullptr;

	index_type index = GetIndex(_handle);
	size_t version = GetVersion(_handle);

	HDL_ASSERT(index < getNodeBufferSize());
	auto node = m_nodeBuffer + index;

	if (node->m_version != version)
		return nullptr; // The handle was already destroyed.

	HDL_ASSERT(node->m_allocated);
	return &node->m_value;
}

template<typename T, typename IntegerType, size_t MaxHandles>
size_t
HandlePool<T, IntegerType, MaxHandles>::getNodeBufferSize() const
{
	return m_nodeBufferSizeBytes / sizeof(Node);
}

template<typename T, typename IntegerType, size_t MaxHandles>
typename HandlePool<T, IntegerType, MaxHandles>::index_type
HandlePool<T, IntegerType, MaxHandles>::GetIndex(integer_type _handle)
{
	// The index is in the low bits of the handle.
	return _handle & kIndexMask;
}

template<typename T, typename IntegerType, size_t MaxHandles>
size_t 
HandlePool<T, IntegerType, MaxHandles>::GetVersion(integer_type _handle)
{
	// The version is in the high bits of the handle.
	// Note: integer_type must be unsigned otherwise this would do an arithmetic shift instead of logical shift.
	return _handle >> kIndexNumBits;
}

template<typename T, typename IntegerType, size_t MaxHandles>
typename HandlePool<T, IntegerType, MaxHandles>::integer_type
HandlePool<T, IntegerType, MaxHandles>::GetID(index_type _index, size_t _version)
{
	return (integer_type)((_version << kIndexNumBits) + _index);
}