#ifndef BOOSTTHREAD_MOCKUP_HH
#define BOOSTTHREAD_MOCKUP_HH
namespace boost {
namespace thread {
 int hardware_concurrency();
}
struct mutex {};
struct condition_variable {
void notify_all(){}
};
template<class T>
struct unique_lock {
template<class T2>
unique_lock(T2){}
};
template<class T>
struct lock_guard {
template<class T2>
lock_guard(T2){}
};
}
#endif //BOOSTTHREAD_MOCKUP_HH