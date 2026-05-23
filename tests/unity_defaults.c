/* Weak default setUp/tearDown for Unity tests that don't supply their own.
 *
 * Newer Unity releases require both to be defined at link time. The
 * legacy PlatformIO test runner injected empty stubs automatically;
 * we recreate that behaviour by linking this shim into every test and
 * relying on weak linkage to let individual test files override.
 *
 * Used by test_mining_pool/test_main.cpp (which never defined them).
 * Tests like test_utils/test_main.cpp do supply their own setUp/tearDown
 * — those override the weak stubs without a multiple-definition error.
 */

#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}
#else
#  error "Unsupported compiler for weak setUp/tearDown stubs"
#endif
