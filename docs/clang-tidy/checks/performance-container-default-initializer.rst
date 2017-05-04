.. title:: clang-tidy - performance-container-default-initializer

performance-container-default-initializer
=========================================

This check will warn when one default-initialises a standard container and
immediately after it adds elements to that. In these cases the container
should be initialised in the row declared.
For instance:

.. code-block:: c++

   int foo = 5;
   std::vector<int> vec;
   vec.push_back(2);
   vec.push_back(1.0);
   vec.emplace_back(foo);


Should be refactored into a more efficient form:

.. code-block:: c++

   int foo = 5;
   std::vector<int> vec{2, (int)1.0, foo};

This checker works with the following containers: ``std::vector``, ``std::map``,
``std::deque`` and ``std::set``, with operations of ``emplace_back()``, ``emplace()``
, ``push_back()`` and ``insert()``. It's also able to fix simple cases via 
``FixItHints`` taking narrowing conversions into consideration.