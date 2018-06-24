# Handle

Handles (in general) are a way of referencing an object with an integer. It's a little bit like pointers, except the additional indirection
they introduce can be leveraged to implement a way of determining if the object is still valid before accessing it.

This can be useful for a variety of reasons, but basically, any time controlling who owns pointers to the object is not possible
or too complicated, a handle system could be useful.

```c++
using EntityID = Handle<Entity>;

void SpawnEntity(const SpawnData& data)
{
    // Create an entity and return a handle to it
    EntityID id = EntityID::Create(data);

    QueueEntityForProcessing(id);
}

...

// Executed potentially much later
void UnqueueAndProcessEntity(EntityID id)
{
    if (Entity* entity = EntityID::Get(id))
    {
        entity->process();
    }
    else
    {
        // Oh no, the entity was destroyed in the meantime
        goHaveADrink();
    }
}
```
## Under the hood

Each handle is made of a number of bits used as an index in an array (that's where the referenced object is), 
and some bits acting as a version number. If the version number in the handle matches the one in the array at the same index, the object is valid.
Otherwise it means the object has been destroyed.

Now, that's also pretty much what all the other handle implementations do. Here are the advantages of this one:

### It's not so opaque

The provided **natvis** files mean that - if you use Visual Studio - you can see the value behind a handle at anytime in the debugger.

### It's resizable AND thread-safe

One very common limitation of this kind of handle system, is that it needs to allocate the array where the objects
are stored during initialization. And to be able to resize this array means you have to be sure no one is reading it, 
so it's usually either not thread-safe, or not resizable (or you'd need to lock every time you access any object, but that's not very appealing).

This implementation uses virtual memory to reserve enough address space to store all the objects you could fit the index bits of the handle,
but only commits the memory that you need to store the current number of objects, and can commit more as needed. It never shrinks though.

Accessing an object from a handle is lock-free, it doesn't need any synchronization since growing the array does not move existing objects.
It's also very fast since it's just indexing an array.
Creating/destroying handles does use locks however, but they are short enough.

### It's stongly typed

Handles are not typedefs to integers, they are a class, which is great for type-safety.

The `Tag` template parameter also means you can have two handles to the same object type behave as different types.

```c++
using TextureID = Handle<Texture>;
using DebugTextureID = Handle<Texture, Debug>; // provided that Debug is a type
// Can't pass a TextureID to a function taking a DebugTextureID.
```

### It's customizable

The template parameters can be used to choose the type of integer to use, and the number of bits for the index/version 
(although kind of indirectly: `MaxHandles` will influence the number of index bits, and the version will use all the 
remaining bits in the choosen integer type).

```c++
using ObjectHandle = Handle<Object, void, uint16_t, 512>;
// ObjectHandles take 2 bytes (they're uint16_t) and there can be only 512 hanles in flight (which means 9 bits of index and 7 bits of version)
```
