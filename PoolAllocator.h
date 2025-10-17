#pragma once

#include <cstddef>
#include <new>

template<class T, size_t MAXSIZE> class PoolAllocator
{
public:
	PoolAllocator()
	{
		for (size_t i = 0; i < MAXSIZE - 1; ++i)
		{
			pool[i].next = &pool[i + 1];
		}
		pool[MAXSIZE - 1].next = nullptr;
		freeListHead = &pool[0];
	}

	~PoolAllocator() {}

	T* Alloc()
	{
		if (freeListHead)
		{
			Slot* head = freeListHead;
			freeListHead = head->next;

			return new(&head->element) T();
		}
		return nullptr;
	}

	void Free(T* addr)
	{
		if (addr == nullptr)
		{
			return;
		}

		addr->~T();

		Slot* slot = reinterpret_cast<Slot*>(addr);
		slot->next = freeListHead;
		freeListHead = slot;
	}

private:
	union Slot
	{
		T element;
		Slot* next;

		Slot() {}
		~Slot() {}
	};

	Slot pool[MAXSIZE];
	Slot* freeListHead;

	PoolAllocator(const PoolAllocator&) = delete;
	PoolAllocator& operator=(const PoolAllocator&) = delete;
};
