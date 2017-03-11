.. title:: clang-tidy - performance-inefficient-shared-pointer-reference

performance-inefficient-shared-pointer-reference
================================================

Passing an ``std::shared_ptr<Derived>`` type as an argument to a function expecting ``std::shared_ptr<Base>``
(where ``Derived`` is a derived class of ``Base``) is highly inefficient. In order to convert ``std::shared_ptr<Derived>``
to ``std::shared_ptr<Base>`` calling the ``Base`` class's converting constructor is necessary.

Consider this code:

.. code-block:: c++

    struct Base {};
    struct Derived : Base {};

    void function(std::shared_ptr<Base> ptr);

    int main() {
        auto ptr = std::make_shared<Derived>();
        function(ptr);
    }


This could be rewritten as:

.. code-block:: c++

    struct Base {};
    struct Derived : Base {};

    void function(const Base& ptr);

    int main() {
        auto ptr = std::make_shared<Derived>();
        function(*ptr.get());
    }

The latter code is about a magnitude faster.