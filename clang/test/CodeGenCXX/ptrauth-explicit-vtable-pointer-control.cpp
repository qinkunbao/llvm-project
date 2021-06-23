// RUN: %clang_cc1 %s -x c++ -std=c++11  -triple arm64-apple-ios -fptrauth-intrinsics -fptrauth-calls -emit-llvm -O1 -disable-llvm-passes  -o - | FileCheck  --check-prefix=CHECK-DEFAULT-NONE %s
// RUN: %clang_cc1 %s -x c++ -std=c++11  -triple arm64-apple-ios -fptrauth-intrinsics -fptrauth-calls -fptrauth-vtable-pointer-type-discrimination -emit-llvm -O1 -disable-llvm-passes -o - | FileCheck --check-prefix=CHECK-DEFAULT-TYPE %s
// RUN: %clang_cc1 %s -x c++ -std=c++11  -triple arm64-apple-ios -fptrauth-intrinsics -fptrauth-calls -fptrauth-vtable-pointer-address-discrimination -emit-llvm -O1 -disable-llvm-passes -o - | FileCheck --check-prefix=CHECK-DEFAULT-ADDRESS %s
// RUN: %clang_cc1 %s -x c++ -std=c++11  -triple arm64-apple-ios -fptrauth-intrinsics -fptrauth-calls -fptrauth-vtable-pointer-type-discrimination -fptrauth-vtable-pointer-address-discrimination -emit-llvm -O1 -disable-llvm-passes -o - | FileCheck --check-prefix=CHECK-DEFAULT-BOTH %s
#include <ptrauth.h>
namespace test1 {

#define authenticated(a...) ptrauth_cxx_vtable_pointer(a)

struct NoExplicitAuth {
  virtual ~NoExplicitAuth();
  virtual void f();
  virtual void g();
};

struct authenticated(no_authentication, no_address_discrimination, no_extra_discrimination) ExplicitlyDisableAuth {
  virtual ~ExplicitlyDisableAuth();
  virtual void f();
  virtual void g();
};

struct authenticated(default_key, address_discrimination, default_extra_discrimination) ExplicitAddressDiscrimination {
  virtual ~ExplicitAddressDiscrimination();
  virtual void f();
  virtual void g();
};

struct authenticated(default_key, no_address_discrimination, default_extra_discrimination) ExplicitNoAddressDiscrimination {
  virtual ~ExplicitNoAddressDiscrimination();
  virtual void f();
  virtual void g();
};

struct authenticated(default_key, default_address_discrimination, no_extra_discrimination) ExplicitNoExtraDiscrimination {
  virtual ~ExplicitNoExtraDiscrimination();
  virtual void f();
  virtual void g();
};

struct authenticated(default_key, default_address_discrimination, type_discrimination) ExplicitTypeDiscrimination {
  virtual ~ExplicitTypeDiscrimination();
  virtual void f();
  virtual void g();
};

struct authenticated(default_key, default_address_discrimination, custom_discrimination, 0xf00d) ExplicitCustomDiscrimination {
  virtual ~ExplicitCustomDiscrimination();
  virtual void f();
  virtual void g();
};

template <typename T>
struct SubClass : T {
  virtual void g();
  virtual T *h();
};

template <typename T>
SubClass<T> *make_subclass(T *);

struct authenticated(default_key, address_discrimination, type_discrimination) BasicStruct {
  virtual ~BasicStruct();
};

template <typename T>
struct PrimaryBasicStruct : BasicStruct, T {};
template <typename T>
struct PrimaryBasicStruct<T> *make_primary_basic(T *);
template <typename T>
struct SecondaryBasicStruct : T, BasicStruct {};
template <typename T>
struct SecondaryBasicStruct<T> *make_secondary_basic(T *);
template <typename T>
struct VirtualSubClass : virtual T {
  virtual void g();
  virtual T *h();
};
template <typename T>
struct VirtualPrimaryStruct : virtual T, VirtualSubClass<T> {};
template <typename T>
struct VirtualPrimaryStruct<T> *make_virtual_primary(T *);
template <typename T>
struct VirtualSecondaryStruct : VirtualSubClass<T>, virtual T {};
template <typename T>
struct VirtualSecondaryStruct<T> *make_virtual_secondary(T *);

void test(NoExplicitAuth *a, ExplicitlyDisableAuth *b, ExplicitAddressDiscrimination *c,
          ExplicitNoAddressDiscrimination *d, ExplicitNoExtraDiscrimination *e,
          ExplicitTypeDiscrimination *f, ExplicitCustomDiscrimination *g) {
  a->f();
  // CHECK-DEFAULT-NONE: %0 = load %"struct.test1::NoExplicitAuth"*, %"struct.test1::NoExplicitAuth"** %a.addr, align 8
  // CHECK-DEFAULT-NONE: %1 = bitcast %"struct.test1::NoExplicitAuth"* %0 to void (%"struct.test1::NoExplicitAuth"*)***
  // CHECK-DEFAULT-NONE: %vtable = load void (%"struct.test1::NoExplicitAuth"*)**, void (%"struct.test1::NoExplicitAuth"*)*** %1, align 8
  // CHECK-DEFAULT-NONE: %2 = ptrtoint void (%"struct.test1::NoExplicitAuth"*)** %vtable to i64
  // CHECK-DEFAULT-NONE: %3 = call i64 @llvm.ptrauth.auth(i64 %2, i32 2, i64 0)
  // CHECK-DEFAULT-NONE: %4 = inttoptr i64 %3 to void (%"struct.test1::NoExplicitAuth"*)**
  // CHECK-DEFAULT-NONE: %vfn = getelementptr inbounds void (%"struct.test1::NoExplicitAuth"*)*, void (%"struct.test1::NoExplicitAuth"*)** %4, i64 2

  b->f();
  // CHECK-DEFAULT-NONE: %8 = load %"struct.test1::ExplicitlyDisableAuth"*, %"struct.test1::ExplicitlyDisableAuth"** %b.addr, align 8
  // CHECK-DEFAULT-NONE: %9 = bitcast %"struct.test1::ExplicitlyDisableAuth"* %8 to void (%"struct.test1::ExplicitlyDisableAuth"*)***
  // CHECK-DEFAULT-NONE: %vtable1 = load void (%"struct.test1::ExplicitlyDisableAuth"*)**, void (%"struct.test1::ExplicitlyDisableAuth"*)*** %9, align 8
  // CHECK-DEFAULT-NONE: %vfn2 = getelementptr inbounds void (%"struct.test1::ExplicitlyDisableAuth"*)*, void (%"struct.test1::ExplicitlyDisableAuth"*)** %vtable1, i64 2

  c->f();
  // CHECK-DEFAULT-NONE: %13 = load %"struct.test1::ExplicitAddressDiscrimination"*, %"struct.test1::ExplicitAddressDiscrimination"** %c.addr, align 8
  // CHECK-DEFAULT-NONE: %14 = bitcast %"struct.test1::ExplicitAddressDiscrimination"* %13 to void (%"struct.test1::ExplicitAddressDiscrimination"*)***
  // CHECK-DEFAULT-NONE: %vtable3 = load void (%"struct.test1::ExplicitAddressDiscrimination"*)**, void (%"struct.test1::ExplicitAddressDiscrimination"*)*** %14, align 8
  // CHECK-DEFAULT-NONE: %15 = ptrtoint %"struct.test1::ExplicitAddressDiscrimination"* %13 to i64
  // CHECK-DEFAULT-NONE: %16 = ptrtoint void (%"struct.test1::ExplicitAddressDiscrimination"*)** %vtable3 to i64
  // CHECK-DEFAULT-NONE: %17 = call i64 @llvm.ptrauth.auth(i64 %16, i32 2, i64 %15)
  // CHECK-DEFAULT-NONE: %18 = inttoptr i64 %17 to void (%"struct.test1::ExplicitAddressDiscrimination"*)**
  // CHECK-DEFAULT-NONE: %vfn4 = getelementptr inbounds void (%"struct.test1::ExplicitAddressDiscrimination"*)*, void (%"struct.test1::ExplicitAddressDiscrimination"*)** %18, i64 2

  d->f();
  // CHECK-DEFAULT-NONE: %22 = load %"struct.test1::ExplicitNoAddressDiscrimination"*, %"struct.test1::ExplicitNoAddressDiscrimination"** %d.addr, align 8
  // CHECK-DEFAULT-NONE: %23 = bitcast %"struct.test1::ExplicitNoAddressDiscrimination"* %22 to void (%"struct.test1::ExplicitNoAddressDiscrimination"*)***
  // CHECK-DEFAULT-NONE: %vtable5 = load void (%"struct.test1::ExplicitNoAddressDiscrimination"*)**, void (%"struct.test1::ExplicitNoAddressDiscrimination"*)*** %23, align 8
  // CHECK-DEFAULT-NONE: %24 = ptrtoint void (%"struct.test1::ExplicitNoAddressDiscrimination"*)** %vtable5 to i64
  // CHECK-DEFAULT-NONE: %25 = call i64 @llvm.ptrauth.auth(i64 %24, i32 2, i64 0)
  // CHECK-DEFAULT-NONE: %26 = inttoptr i64 %25 to void (%"struct.test1::ExplicitNoAddressDiscrimination"*)**
  // CHECK-DEFAULT-NONE: %vfn6 = getelementptr inbounds void (%"struct.test1::ExplicitNoAddressDiscrimination"*)*, void (%"struct.test1::ExplicitNoAddressDiscrimination"*)** %26, i64 2

  e->f();
  // CHECK-DEFAULT-NONE: %30 = load %"struct.test1::ExplicitNoExtraDiscrimination"*, %"struct.test1::ExplicitNoExtraDiscrimination"** %e.addr, align 8
  // CHECK-DEFAULT-NONE: %31 = bitcast %"struct.test1::ExplicitNoExtraDiscrimination"* %30 to void (%"struct.test1::ExplicitNoExtraDiscrimination"*)***
  // CHECK-DEFAULT-NONE: %vtable7 = load void (%"struct.test1::ExplicitNoExtraDiscrimination"*)**, void (%"struct.test1::ExplicitNoExtraDiscrimination"*)*** %31, align 8
  // CHECK-DEFAULT-NONE: %32 = ptrtoint void (%"struct.test1::ExplicitNoExtraDiscrimination"*)** %vtable7 to i64
  // CHECK-DEFAULT-NONE: %33 = call i64 @llvm.ptrauth.auth(i64 %32, i32 2, i64 0)
  // CHECK-DEFAULT-NONE: %34 = inttoptr i64 %33 to void (%"struct.test1::ExplicitNoExtraDiscrimination"*)**
  // CHECK-DEFAULT-NONE: %vfn8 = getelementptr inbounds void (%"struct.test1::ExplicitNoExtraDiscrimination"*)*, void (%"struct.test1::ExplicitNoExtraDiscrimination"*)** %34, i64 2

  f->f();
  // CHECK-DEFAULT-NONE: %38 = load %"struct.test1::ExplicitTypeDiscrimination"*, %"struct.test1::ExplicitTypeDiscrimination"** %f.addr, align 8
  // CHECK-DEFAULT-NONE: %39 = bitcast %"struct.test1::ExplicitTypeDiscrimination"* %38 to void (%"struct.test1::ExplicitTypeDiscrimination"*)***
  // CHECK-DEFAULT-NONE: %vtable9 = load void (%"struct.test1::ExplicitTypeDiscrimination"*)**, void (%"struct.test1::ExplicitTypeDiscrimination"*)*** %39, align 8
  // CHECK-DEFAULT-NONE: %40 = ptrtoint void (%"struct.test1::ExplicitTypeDiscrimination"*)** %vtable9 to i64
  // CHECK-DEFAULT-NONE: %41 = call i64 @llvm.ptrauth.auth(i64 %40, i32 2, i64 6177)
  // CHECK-DEFAULT-NONE: %42 = inttoptr i64 %41 to void (%"struct.test1::ExplicitTypeDiscrimination"*)**
  // CHECK-DEFAULT-NONE: %vfn10 = getelementptr inbounds void (%"struct.test1::ExplicitTypeDiscrimination"*)*, void (%"struct.test1::ExplicitTypeDiscrimination"*)** %42, i64 2

  g->f();
  // CHECK-DEFAULT-NONE: %46 = load %"struct.test1::ExplicitCustomDiscrimination"*, %"struct.test1::ExplicitCustomDiscrimination"** %g.addr, align 8
  // CHECK-DEFAULT-NONE: %47 = bitcast %"struct.test1::ExplicitCustomDiscrimination"* %46 to void (%"struct.test1::ExplicitCustomDiscrimination"*)***
  // CHECK-DEFAULT-NONE: %vtable11 = load void (%"struct.test1::ExplicitCustomDiscrimination"*)**, void (%"struct.test1::ExplicitCustomDiscrimination"*)*** %47, align 8
  // CHECK-DEFAULT-NONE: %48 = ptrtoint void (%"struct.test1::ExplicitCustomDiscrimination"*)** %vtable11 to i64
  // CHECK-DEFAULT-NONE: %49 = call i64 @llvm.ptrauth.auth(i64 %48, i32 2, i64 61453)
  // CHECK-DEFAULT-NONE: %50 = inttoptr i64 %49 to void (%"struct.test1::ExplicitCustomDiscrimination"*)**
  // CHECK-DEFAULT-NONE: %vfn12 = getelementptr inbounds void (%"struct.test1::ExplicitCustomDiscrimination"*)*, void (%"struct.test1::ExplicitCustomDiscrimination"*)** %50, i64 2

  // basic subclass
  make_subclass(a)->f();
  // CHECK-DEFAULT-NONE: %vtable13 = load void (%"struct.test1::NoExplicitAuth"*)**, void (%"struct.test1::NoExplicitAuth"*)*** %56, align 8
  // CHECK-DEFAULT-NONE: %57 = ptrtoint void (%"struct.test1::NoExplicitAuth"*)** %vtable13 to i64
  // CHECK-DEFAULT-NONE: %58 = call i64 @llvm.ptrauth.auth(i64 %57, i32 2, i64 0)
  // CHECK-DEFAULT-NONE: %59 = inttoptr i64 %58 to void (%"struct.test1::NoExplicitAuth"*)**
  // CHECK-DEFAULT-NONE: %vfn14 = getelementptr inbounds void (%"struct.test1::NoExplicitAuth"*)*, void (%"struct.test1::NoExplicitAuth"*)** %59, i64 2

  make_subclass(a)->g();
  // CHECK-DEFAULT-NONE: %64 = bitcast %"struct.test1::SubClass"* %call15 to void (%"struct.test1::SubClass"*)***
  // CHECK-DEFAULT-NONE: %vtable16 = load void (%"struct.test1::SubClass"*)**, void (%"struct.test1::SubClass"*)*** %64, align 8
  // CHECK-DEFAULT-NONE: %65 = ptrtoint void (%"struct.test1::SubClass"*)** %vtable16 to i64
  // CHECK-DEFAULT-NONE: %66 = call i64 @llvm.ptrauth.auth(i64 %65, i32 2, i64 0)
  // CHECK-DEFAULT-NONE: %67 = inttoptr i64 %66 to void (%"struct.test1::SubClass"*)**
  // CHECK-DEFAULT-NONE: %vfn17 = getelementptr inbounds void (%"struct.test1::SubClass"*)*, void (%"struct.test1::SubClass"*)** %67, i64 3

  make_subclass(a)->h();
  // CHECK-DEFAULT-NONE: %72 = bitcast %"struct.test1::SubClass"* %call18 to %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)***
  // CHECK-DEFAULT-NONE: %vtable19 = load %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)**, %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)*** %72, align 8
  // CHECK-DEFAULT-NONE: %73 = ptrtoint %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)** %vtable19 to i64
  // CHECK-DEFAULT-NONE: %74 = call i64 @llvm.ptrauth.auth(i64 %73, i32 2, i64 0)
  // CHECK-DEFAULT-NONE: %75 = inttoptr i64 %74 to %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)**
  // CHECK-DEFAULT-NONE: %vfn20 = getelementptr inbounds %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)*, %"struct.test1::NoExplicitAuth"* (%"struct.test1::SubClass"*)** %75, i64 4

  make_subclass(b)->f();
  // CHECK-DEFAULT-NONE: %vtable23 = load void (%"struct.test1::ExplicitlyDisableAuth"*)**, void (%"struct.test1::ExplicitlyDisableAuth"*)*** %81, align 8
  // CHECK-DEFAULT-NONE: %vfn24 = getelementptr inbounds void (%"struct.test1::ExplicitlyDisableAuth"*)*, void (%"struct.test1::ExplicitlyDisableAuth"*)** %vtable23, i64 2

  make_subclass(b)->g();
  // CHECK-DEFAULT-NONE: %vtable26 = load void (%"struct.test1::SubClass.0"*)**, void (%"struct.test1::SubClass.0"*)*** %86, align 8
  // CHECK-DEFAULT-NONE: %vfn27 = getelementptr inbounds void (%"struct.test1::SubClass.0"*)*, void (%"struct.test1::SubClass.0"*)** %vtable26, i64 3

  make_subclass(b)->h();

  make_subclass(c)->f();
  make_subclass(c)->g();
  make_subclass(c)->h();

  make_subclass(d)->f();
  make_subclass(d)->g();
  make_subclass(d)->h();

  make_subclass(e)->f();
  make_subclass(e)->g();
  make_subclass(e)->h();

  make_subclass(f)->f();
  make_subclass(f)->g();
  make_subclass(f)->h();

  make_subclass(g)->f();
  make_subclass(g)->g();
  make_subclass(g)->h();

  // Basic multiple inheritance
  make_primary_basic(a)->f();
  make_primary_basic(b)->f();
  make_primary_basic(c)->f();
  make_primary_basic(d)->f();
  make_primary_basic(e)->f();
  make_primary_basic(f)->f();
  make_primary_basic(g)->f();
  make_secondary_basic(a)->f();
  make_secondary_basic(b)->f();
  make_secondary_basic(c)->f();
  make_secondary_basic(d)->f();
  make_secondary_basic(e)->f();
  make_secondary_basic(f)->f();
  make_secondary_basic(g)->f();

  // virtual inheritance
  make_virtual_primary(a)->f();
  make_virtual_primary(b)->f();
  make_virtual_primary(c)->f();
  make_virtual_primary(d)->f();
  make_virtual_primary(e)->f();
  make_virtual_primary(f)->f();
  make_virtual_primary(g)->f();
  make_virtual_secondary(a)->f();
  make_virtual_secondary(b)->f();
  make_virtual_secondary(c)->f();
  make_virtual_secondary(d)->f();
  make_virtual_secondary(e)->f();
  make_virtual_secondary(f)->f();
  make_virtual_secondary(g)->f();
}
} // namespace test1

// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 37831)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 2191)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 44989)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 63209)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43275)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 19073)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 25182)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 23051)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 3267)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 57764)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 8498)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61320)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 7682)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 53776)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 49565)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6177)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 61453)
// CHECK-DEFAULT-TYPE:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)

// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 37831)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 2191)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 44989)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 63209)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43275)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 19073)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 25182)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 23051)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 3267)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 57764)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 8498)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61320)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 7682)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 53776)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 0)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-ADDRESS:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)

// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 37831)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 2191)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 44989)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 63209)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43275)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 19073)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 25182)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 23051)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 3267)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 57764)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 8498)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61320)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 7682)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 53776)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 49565)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 27707)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 31119)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 56943)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 5268)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 6022)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 34147)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 39413)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 6177)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 29468)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 61453)
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[T:%.*]], i32 2, i64 [[T:%.*]])
// CHECK-DEFAULT-BOTH:   [[T:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T:%.*]], i64 43175)
