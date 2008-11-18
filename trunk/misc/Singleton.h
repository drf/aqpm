#include <QtCore/QAtomicPointer>
#include <sys/types.h>

typedef void (*KdeCleanUpFunction)();

/**
 * @internal
 *
 * Helper class for K_GLOBAL_STATIC to clean up the object on library unload or application
 * shutdown.
 */
class KCleanUpGlobalStatic
{
    public:
        KdeCleanUpFunction func;

        inline ~KCleanUpGlobalStatic() { func(); }
};

#define AQPM_GLOBAL_STATIC_STRUCT_NAME(NAME)

#define AQPM_GLOBAL_STATIC(TYPE, NAME) AQPM_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ())

#define AQPM_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                            \
static QBasicAtomicPointer<TYPE > _k_static_##NAME = Q_BASIC_ATOMIC_INITIALIZER(0); \
static bool _k_static_##NAME##_destroyed;                                      \
static struct AQPM_GLOBAL_STATIC_STRUCT_NAME(NAME)                                \
{                                                                              \
    inline bool isDestroyed() const                                            \
    {                                                                          \
        return _k_static_##NAME##_destroyed;                                   \
    }                                                                          \
    inline bool exists() const                                                 \
    {                                                                          \
        return _k_static_##NAME != 0;                                          \
    }                                                                          \
    inline operator TYPE*()                                                    \
    {                                                                          \
        return operator->();                                                   \
    }                                                                          \
    inline TYPE *operator->()                                                  \
    {                                                                          \
        if (!_k_static_##NAME) {                                               \
            if (isDestroyed()) {                                               \
                qFatal("Fatal Error: Accessed global static '%s *%s()' after destruction. " \
                       "Defined at %s:%d", #TYPE, #NAME, __FILE__, __LINE__);  \
            }                                                                  \
            TYPE *x = new TYPE ARGS;                                           \
            if (!_k_static_##NAME.testAndSetOrdered(0, x)                      \
                && _k_static_##NAME != x ) {                                   \
                delete x;                                                      \
            } else {                                                           \
                static KCleanUpGlobalStatic cleanUpObject = { destroy };       \
            }                                                                  \
        }                                                                      \
        return _k_static_##NAME;                                               \
    }                                                                          \
    inline TYPE &operator*()                                                   \
    {                                                                          \
        return *operator->();                                                  \
    }                                                                          \
    static void destroy()                                                      \
    {                                                                          \
        _k_static_##NAME##_destroyed = true;                                   \
        TYPE *x = _k_static_##NAME;                                            \
        _k_static_##NAME = 0;                                                  \
        delete x;                                                              \
    }                                                                          \
} NAME;
