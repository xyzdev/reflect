/*

Copyright (c) 2016 xyzdev.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef XYZDEV_REFLECTION_HPP
#define XYZDEV_REFLECTION_HPP

#include "json.hpp"
#include <sstream>
#include <map>
#include <type_traits>

/**
 * Reflector: Reads or writes a member to/from json::Element.
 * Reflection: Visits each reflector and decides when to make them read, write or call the underlying member.
 * Reflectable: Any class with a reflect method (void reflect(Reflection &)). Uses XYZ_REFLECT(...) et al. to create reflectors and pass them to reflection.
 */

namespace xyz {
  namespace core {

    typedef unsigned TypeId;

    namespace detail {

      template<typename Src>
      json::String toString(const Src &src) {
        std::ostringstream os;
        os << src;
        return os.str();
      }

      template<typename Dest>
      Dest fromString(const json::String &src) {
        std::ostringstream is(src);
        Dest dest = Dest();
        is >> dest;
        return dest;
      }

      template<>
      inline json::String toString(const json::String &src) {
        return src;
      }

      template<>
      inline json::String fromString(const json::String &src) {
        return src;
      }

      struct TypeIdGenerator {
          static TypeId next() {
            static TypeId lastId = 0;
            return ++lastId;
          }
      };

    }

    template<typename T>
    static TypeId type_id() {
      static TypeId id = detail::TypeIdGenerator::next();
      return id;
    }

    class AbstractReflector {
    public:
      virtual json::Element read() = 0;
      virtual void write(const json::Element &data) = 0;
      virtual bool isMethod() { return false; }

      virtual json::Element call(const json::Array &data) {
        throw json::TypeError();
      }
    };

    template<typename Field, typename UnusedSpecializationArg=void>
    class Reflector: public AbstractReflector {
    public:
      typedef Field field_type;

      Reflector(Field &field)
        :field(field) {}

      json::Element read() {
        return json::Element(detail::toString(field));
      }

      void write(const json::Element &data) {
        if(data.getType() != json::Element::NULL_VALUE) {
          field = detail::fromString<Field>(data.str());
        } else {
          field = Field();
        }
      }

    protected:
      Field &field;
    };

    template<>
    class Reflector<void>: public AbstractReflector {
    public:
      typedef void field_type;

      virtual json::Element read() {
        return json::Element::NULL_VALUE;
      }

      virtual void write(const json::Element &data) {
        if(!data.isNull()) throw json::TypeError("TypeError: Tried to write to void type.");
      }
    };

    template<>
    class Reflector<json::String, void>: public AbstractReflector {
    public:
      typedef json::String field_type;

      Reflector(field_type &field)
        :field(field) {}

      json::Element read() {
        return json::Element(field);
      }

      void write(const json::Element &data) {
        if(data.getType() != json::Element::NULL_VALUE) {
          field = data.str();
        } else {
          field = field_type();
        }
      }

    protected:
        field_type &field;
    };

    template<typename Field>
    class Reflector<Field, typename std::enable_if<std::is_integral<Field>::value>::type>: public AbstractReflector {
    public:
      Reflector(Field &field):field(field) {}

      json::Element read() {
        return json::Element(json::Number(field));
      }

      void write(const json::Element &data) {
        field = Field(data.number());
      }

    protected:
      Field &field;
    };

    template<typename Field>
    class Reflector<Field, typename std::enable_if<std::is_floating_point<Field>::value>::type>: public AbstractReflector {
    public:
      Reflector(Field &field):field(field) {}

      json::Element read() {
        return json::Element(json::Number(field));
      }

      void write(const json::Element &data) {
        field = Field(data.number());
      }

    protected:
      Field &field;
    };

    template<> inline json::Element Reflector<bool>::read() {
      return json::Element(json::Boolean(field));
    }

    template<> inline void Reflector<bool>::write(const json::Element &data) {
      field = (data.getType() != json::Element::NULL_VALUE) ? bool(data.boolean()) : bool();
    }

    template<> inline json::Element Reflector<json::Element>::read() {
      return field;
    }

    template<> inline void Reflector<json::Element>::write(const json::Element &data) {
      field = data;
    }

    // TODO: This breaks non-collection templated fields.
    template<template<typename ...> class Container, typename ... Args>
    class Reflector< Container<Args...> >: public AbstractReflector {
    public:
      typedef Container<Args...> field_type;
      typedef typename field_type::value_type element_type;

      Reflector(field_type &field)
        :field(field) {}

      json::Element read() {
        json::Array array;
        for(typename field_type::iterator i = field.begin(); i != field.end(); ++i) {
          Reflector<element_type> refl(*i);
          array.push_back(refl.read());
        }
        return json::Element(array);
      };

      void write(const json::Element &data) {
        std::vector<element_type> v;
        if(data.getType() != json::Element::NULL_VALUE) {
          for(json::Array::const_iterator i = data.array().begin(); i != data.array().end(); ++i) {
            element_type elem;
            Reflector<element_type> refl(elem);
            refl.write(*i);
            v.push_back(elem);
          }
        }
        field = field_type(v.begin(), v.end());
      }

    protected:
      field_type &field;
    };

    template<typename Key, typename Value>
    class Reflector< std::map<Key, Value> >: public AbstractReflector {
    public:
      typedef std::map<Key, Value> field_type;

      Reflector(field_type &field)
        :field(field) {}

      json::Element read() {
        json::Object obj;
        for(typename field_type::iterator i = field.begin(); i != field.end(); ++i) {
          Reflector<Value> refl(i->second);
          obj[detail::toString(i->first)] = refl.read();
        }
        return json::Element(obj);
      }

      void write(const json::Element &data) {
        field.clear();
        if(data.getType() != json::Element::NULL_VALUE) {
          for(json::Object::const_iterator i = data.object().begin(); i != data.object().end(); ++i) {
            Value elem;
            Reflector<Value> refl(elem);
            refl.write(i->second);
            field[detail::fromString<Key>(i->first)] = elem;
          }
        }
      }

    protected:
      field_type &field;
    };

    template<typename Class, typename Property>
    class PropertyReflector: public AbstractReflector {
    public:
      PropertyReflector(Class &instance, Property (Class::*getter)(), void (Class::*setter)(Property))
        :instance(instance),
         getter(getter),
         setter(setter)
      { }

      json::Element read() {
        Property val((instance.*getter)());
        Reflector<Property> refl(val);
        return refl.read();
      }

      void write(const json::Element &data) {
        Property val((instance.*getter)());
        Reflector<Property> refl(val);
        refl.write(data);
        (instance.*setter)(val);
      }

    protected:
      Class &instance;
      Property (Class::*getter)();
      void (Class::*setter)(Property);
    };

    class Reflection {
    public:
      virtual void visit(AbstractReflector &reflector, const char *name) = 0;
    };

    class ReflectionSink: public Reflection {
    public:
      ReflectionSink():methods(false),sink(json::Element::OBJECT) {}

      virtual void visit(AbstractReflector &reflector, const char *name) {
        if(reflector.isMethod() != methods) return;

        json::Element data = reflector.read();
        if(name) {
          sink.object()[name] = data;
        }
        else {
          sink = data;
        }
      }

      bool methods;
      json::Element sink;
    };

    class ReflectionSource: public Reflection {
    public:
      ReflectionSource():source(json::Element::OBJECT) {}
      ReflectionSource(const json::Element &source):source(source) {}

      virtual void visit(AbstractReflector &reflector, const char *name) {
        if(reflector.isMethod()) return;

        if(name) {
          json::Object::iterator it = source.object().find(name);
          if (it != source.object().end()) {
            reflector.write(it->second);
          }
        }
        else {
          reflector.write(source);
        }
      }

      json::Element source;
    };

    class ReflectionCaller: public Reflection {
    public:
      ReflectionCaller(json::String name, json::Array args)
        :name(name),
         args(args),
         found(false) {}

      virtual void visit(xyz::core::AbstractReflector &reflector, const char *name) {
        if(!reflector.isMethod() || this->name != name) { // TODO: Methods in members.
          return;
        }

        found = true;
        result = reflector.call(args);
      }

      json::String name;
      json::Array args;
      json::Element result;
      bool found;
    };

    template<typename T>
    struct can_reflect {
    private:
      template<typename T2>
      static typename std::is_same<decltype(std::declval<T2>().reflect(std::declval<Reflection&>())), void>::type test(int);

      template<typename>
      static std::false_type test(...);

    public:
      static constexpr bool value = decltype(test<T>(0))::value;
    };

    template<typename Field>
    class Reflector<Field, typename std::enable_if<can_reflect<Field>::value>::type>: public AbstractReflector {
    public:

      Reflector(Field &field):field(field) {}

      json::Element read() {
        ReflectionSink sink;
        field.reflect(sink);
        return sink.sink;
      }

      void write(const json::Element &data) {
        if(data.getType() != json::Element::NULL_VALUE) {
          ReflectionSource source(data);
          field.reflect(source);
        } else {
          field = Field();
        }
      }

    protected:
      Field &field;
    };

    namespace detail {

      template<int Idx, typename Arg>
      struct Binding {
        typedef typename std::remove_const<typename std::remove_reference<Arg>::type>::type arg_type;

        static arg_type get(const json::Array &args) {
          auto arg = arg_type();
          Reflector<arg_type> refl(arg);
          refl.write(args[args.size()-(Idx+1)]);
          return arg;
        }
      };

      template<unsigned Count, typename ... Bindings>
      struct Caller {
        template<typename Binding>
        struct Bind {
          typedef Caller<Count + 1, Bindings..., Binding> caller_t;
        };

        static void validate(json::Array::size_type size) {
          if(size != Count) {
            throw json::TypeError("TypeError: Incorrect number of arguments.");
          }
        }

        template<typename Result, typename Class, typename Method>
        struct inner {
          static json::Element call(Class &instance, Method method, const json::Array &args) {
            validate(args.size());
            Result result = (instance.*method)(Bindings::get(args) ...);
            Reflector<Result> refl(result);
            return refl.read();
          }
        };

        template<typename Method, typename Class>
        struct inner<void, Class, Method> {
          static json::Element call(Class &instance, Method method, const json::Array &args) {
            validate(args.size());
            (instance.*method)(Bindings::get(args) ...);
            return json::Element::NULL_VALUE;
          }
        };
      };

      template<int Idx, typename Caller, typename Arg, typename ... Args>
      struct Binder {
        typedef Binding<Idx, Arg> binding;
        typedef typename Caller::template Bind<binding>::caller_t caller;
        typedef typename Binder<Idx - 1, caller, Args ...>::leaf_t leaf_t;
      };

      template<typename Caller, typename Arg, typename ... Args>
      struct Binder<0, Caller, Arg, Args...> {
        typedef Binding<0, Arg> binding;
        typedef typename Caller::template Bind<binding>::caller_t caller;
        typedef Binder<0, Caller, Arg, Args ...> leaf_t;

        template<typename Result, typename Class, typename Method>
        static json::Element call(Class &instance, Method method, const json::Array &args) {
          return (caller::template inner<Result, Class, Method>::call(instance, method, args));
        }
      };

      template<typename T>
      json::Element type() {
        typedef typename std::remove_const<typename std::remove_reference<T>::type>::type real_type;
        auto arg1 = real_type();
        Reflector<real_type> refl(arg1);
        return refl.read().getTypeName();
      }

    }

    template<typename Result, typename Class, typename ... Args>
    class MethodReflector: public AbstractReflector {
    public:
      typedef Result (Class::*method_type)(Args ...);

      MethodReflector(Class &instance, method_type method)
        :instance(instance),
         method(method) {}

      virtual bool isMethod() { return true; }

      virtual json::Element read() {
        json::Array sig;
        sig.push_back("func");
        if(sizeof...(Args) > 0) {
          json::Element types[] = { detail::type<Args>() ... };
          for(unsigned i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
            sig.push_back(types[i]);
          }
        }
        return sig;
      }

      virtual void write(const json::Element &data) {
        throw json::TypeError();
      }

      template<typename Class2, typename ... Argss>
      struct inner {
        static inline json::Element call(Class &instance, method_type method, const json::Array &args) {
          using namespace detail;
          return Binder<sizeof...(Args) - 1, Caller<0>, Argss ...>::leaf_t::template call<Result>(instance, method, args);
        }
      };

      template<typename Class2>
      struct inner<Class2> {
        static inline json::Element call(Class &instance, method_type method, const json::Array &args) {
          using namespace detail;
          return Caller<0>::inner<Result, Class, method_type>::call(instance, method, args);
        }
      };

      virtual json::Element call(const json::Array &args) {
        return inner<Class, Args...>::call(instance, method, args);
      }

      Class &instance;
      method_type method;
    };

    template<typename Field>
    Field &reflect(Reflection &reflection, Field &field, const char *name) {
      Reflector<Field> reflector(field);
      reflection.visit(reflector, name);
      return field;
    }

    template<typename ReflectorClass, class Field>
    Field &reflect_custom(Reflection &reflection, Field &field, const char *name) {
      ReflectorClass reflector(field);
      reflection.visit(reflector, name);
      return field;
    }

    template<typename Class, class Property>
    void reflect_property(Reflection &reflection,
                          Class &instance,
                          Property (Class::*getter)(),
                          void (Class::*setter)(Property),
                          const char *name) {
      PropertyReflector<Class, Property> reflector(instance, getter, setter);
      reflection.visit(reflector, name);
    }

    template<typename Class, typename Result, typename ... Args>
    void reflect_method(Reflection &reflection,
                        Class &instance,
                        Result (Class::*method)(Args ...),
                        const char *name) {
      MethodReflector<Result, Class, Args ...> reflector(instance, method);
      reflection.visit(reflector, name);
    }

  }
}

#define XYZ_REFLECT(reflection, field) (::xyz::core::reflect(reflection, field, #field))
#define XYZ_REFLECT_METHOD(reflection, cls, field) (::xyz::core::reflect_method<cls>(reflection, *this, &cls::field, #field))

#endif
