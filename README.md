Lightweight C++ wrapper around [libxml2][8].

Basically, the core library just implements the absolutely
necessary: error reporting and [resource management][1].

Error reporting is done via exceptions (of course). Thus,
making it hard to 'overlook' an error condition.

The resources that must be managed are memory allocations. Since
the libxml returns raw pointers, the STL's [unique_ptr][2] is a
perfect fit.

As a convenience, the [camel case][3] is converted to [snake case][4] and
the C-style namespace prefixes are tranformed to real C++
namespaces.

Georg Sauthoff <mail@georg.so>

## Unittests

The included unittests are primarily intended as executable
documentation of how to use the libxml API.

Since the C++ wrapper is leightweight, one can easily derive pure
C versions of the test cases (i.e. via inserting prefixes,
conversion to camel case and adding a lot of boilerplate code
for checking return values and freeing memory).

## Documentation

The header is also documents several aspects of the libxml API.

For example, a pointer wrapped in a `std::unique_ptr` means that
a domain dependent free function must be called before scope
exit. A unique pointer passed by values signals that ownership is
transfered. When it is passed by reference, it is still owned by
the caller.

Also, default arguments are made explicit, either via C++ default
arguments or overloads.

In cases, where the return value is just used for error
reporting, it is eliminated - since errors are reported via
exceptions.

## Comfort

Libxml uses for its strings the `xmlChar*` data type, which is
typedeffed to `unsigned char*`.

The libxml examples are thus full of `BAD_CAST` macros
(`BAD_CAST` is defined to `(xmlChar*)`).

Apparently, the motivation behind this was to protect against
accidentally passing non-UTF8 strings into libxml functions
(since [libxml internally uses UTF8][7]).

Thus, to avoid casting hell, the casting is done in the wrapper
functions and they accept `const char *` etc. (or even `const
std::string &` via overloads). This is fine in times where
UTF-8 as internal string encoding is quite popular and a
sane choice.

## Classes

It is tempting to introduce classes and create the wrapper
functions as methods.

For example, for `xmlTextWriter*()` the class could be named
`Text_Writer` and thus methods would be `void Text_Writer::start_document()`
etc.

This would mean to do 3 things at once:

- error reporting
- resource management
- object orientation

But trying to do too much in one function/class is not a good idea.

Thus, creating utility classes is still a good idea - but on top
of the core library.


## License

I don't think that the mechanical wrapper code reaches the
[threshold of originality][5].  In any case, the library code is put into
the public domain and licensed under [CC0][6].


[1]: https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization
[2]: http://en.cppreference.com/w/cpp/memory/unique_ptr
[3]: https://en.wikipedia.org/wiki/CamelCase
[4]: https://en.wikipedia.org/wiki/Snake_case
[5]: https://en.wikipedia.org/wiki/Threshold_of_originality
[6]: https://en.wikipedia.org/wiki/Creative_Commons_license#Zero_.2F_public_domain
[7]: http://www.xmlsoft.org/encoding.html#internal
[8]: http://www.xmlsoft.org/docs.html
