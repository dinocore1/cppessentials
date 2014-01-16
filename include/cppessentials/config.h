
#ifndef CPPESSENTIALS_CONFIG_H_
#define CPPESSENTIALS_CONFIG_H_

#define CPPES_NAMESPACE cpp_essentials

// GCC: compile with -std=c++0x
#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ >= 5))
# define CPPES_HAVE_CXX0X 1
#endif


#endif /* CPPESSENTIALS_CONFIG_H_ */