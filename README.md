# C++ Reflection and Serialization

A simple header based library for reflection and serialization in C++.

### Goals

- Read and write objects to inspectable and serializable elements
- List and call methods
- Zero per-instance overhead (no virtual methods, etc)
- DRY
- No preprocessor magic

### Case

This library was created primarily for use with an [ECS](https://en.wikipedia.org/wiki/Entity_component_system) architecture,
where it can provide serialization of components, scripting language bindings to get and set components
and call methods on systems, as well as a unified way to access data, e.g. for undo/redo in an editor or manual manipulation in a development console.
Since reflection of every component and system class is required to fully enable this, there must be very little effort required per class.
While performance is a minor concern for most of these cases, it is important that performance and memory use/alignment remain unaffected in "ordinary" use.
When working with components, a name or id is normally available which can be mapped to a store or management class,
while it may be acceptable to rely on virtual methods for reflection of systems.

### Solution

Use visitors and template specialization for double dispatch on action (read, write, call) and member type.
An optional macro removes the need to repeat the name of each field, eliminating a potential source of bugs.
Reading, writing and passing arguments is facilitated by an intermediate JSON element on which inspection,
manipulation and serialization can be performed.

## JSON Element and Serialization / Deserialization

The provided `json::Element` class is utilized by the reflection system, however it should be fairly
straight-forward to replace it with your own intermediate data model. It can of course also be used
on its own, without the reflection system.

## Examples

### Reading and writing data

```cpp
struct Component {
    std::string field1;
    std::vector<int> field2;

    // Make reflectable:
    void reflect(xyz::core::Reflection &r) {
        XYZ_REFLECT(r, field1);
        XYZ_REFLECT(r, field2);
    }
};
```

```cpp
// Read:
xyz::core::ReflectionSink sink;
component.reflect(sink);
std::cout << sink.sink.object()["field1"].str();

// Write:
xyz::core::ReflectionSource source;
source.source.object()["field1"] = "new value";
component.reflect(source);
```

### Type id

The `type_id` method template will produce an integer identifying a type for the duration of the
program's execution. Note that it is NOT guaranteed to be stable across runs.

```cpp
xyz::core::type_id<Component>() == xyz::core::type_id<Component>();
xyz::core::type_id<Component>() != xyz::core::type_id<int>();
```

This is useful for safe downcasts without RTTI and dynamic_cast, as well as for looking up manager
classes, etc.

```cpp
class StoreBase {
public:
    virtual ~StoreBase() {}
    virtual int create() = 0;
    virtual xyz::json::Element get(int) = 0;
    virtual void set(int, xyz::json::Element) = 0;
};

template<typename C>
class Store: public StoreBase {
    std::vector<C> components;

    virtual int create();
    virtual xyz::json::Element get(int);
    virtual void set(int, xyz::json::Element);
};

std::map<xyz::core::TypeId, StoreBase*> stores;

auto COMPONENT_ID = xyz::core::type_id<Component>();
stores[COMPONENT_ID] = new Store<Component>();
```

```cpp
int myComponent = stores[COMPONENT_ID]->create();
stores[COMPONENT_ID]->set(myComponent, xyz::json::Element());
```

### Calling methods

```cpp
class System {
public:
    std::vector<Component> foo(int x, int y) { ... }
    void bar() { ... }

    // Make reflectable:
    virtual void reflect(xyz::core::Reflection &r) {
        XYZ_REFLECT_METHOD(r, System, foo);
        XYZ_REFLECT_METHOD(r, System, bar);
    }
};
```

```cpp
// Call method:
xyz::json::Array args(2);
args[0] = xyz::json::Number(42);
args[1] = xyz::json::Number(123);

xyz::core::ReflectionCaller caller("foo", args);
system->reflect(caller);

if(caller.found)
    std::cout << caller.result.array().size();
```

### Reflectors

The `Reflector` class template can be specialized to enable reflection of types which can't be
modified to add a `reflect` method.

```cpp
template<>
class xyz::core::Reflector<Vector3, void>: public xyz::core::AbstractReflector {
public:
    typedef Vector3 field_type;

    Reflector(field_type &field)
        :field(field) {}

    json::Element read() {
        json::Array array(3);
        array[0] = field.x;
        array[1] = field.y;
        array[2] = field.z;
        return json::Element(array);
    }

    void write(const json::Element &data) {
        json::Array array = data.array();
        if(array.size() != 3) {
            throw json::TypeError("Vector3 requires array with three Number elements.");
        }
        field.x = array[0].number();
        field.y = array[1].number();
        field.z = array[2].number();
    }

protected:
    field_type &field;
};
```

Or simply:

```cpp
template<>
xyz::json::Element xyz::core::Reflector<Vector3>::read() {
    json::Array array(3);
    array[0] = field.x;
    array[1] = field.y;
    array[2] = field.z;
    return json::Element(array);
}
template<>
void xyz::core::Reflector<Vector3>::write(const json::Element &data) {
    json::Array array = data.array();
    if(array.size() != 3) {
        throw json::TypeError("Vector3 requires array with three Number elements.");
    }
    field.x = array[0].number();
    field.y = array[1].number();
    field.z = array[2].number();
}
```

### TODO

- Documentation and examples
- Improve readability of code
- Reflector should offer type description, i.e. `{id: <type_id>, type: string/array/etc, [member: {id:...}] }`
- UTF-8 support in JSON serialization
- Function call on member's methods
- Possibly add some syntactic sugar over `Reflection`, e.g. `result = Sink::get(component)`, `Source::set(component, data)`, `result = Caller::call(component, method, args...)`, etc.

## License

Distributed under the [MIT License](LICENSE.md).
