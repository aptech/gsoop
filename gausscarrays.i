/* -----------------------------------------------------------------------------
 * gausscarrays.i
 *
 * SWIG library file containing macros that can be used to manipulate simple
 * pointers as arrays.
 * ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
 * %array_functions(TYPE,NAME)
 *
 * Generates functions for creating and accessing elements of a C array
 * (as pointers).  Creates the following functions:
 *
 *        TYPE *new_NAME(int nelements)
 *        void delete_NAME(TYPE *);
 *        TYPE NAME_getitem(TYPE *, int index);
 *        void NAME_setitem(TYPE *, int index, TYPE value);
 * 
 * ----------------------------------------------------------------------------- */

%define %array_functions(TYPE,NAME)
%inline %{
#include <mteng.h>
%}
%{
static TYPE *new_##NAME(int nelements) { %}
%{  return (TYPE *) GAUSS_Malloc(nelements * sizeof(TYPE)); %}
%{}

static void delete_##NAME(TYPE *ary) { %}
%{  GAUSS_Free(ary); %}
%{}

static TYPE NAME##_getitem(TYPE *ary, int index) {
    return ary[index];
}
static void NAME##_setitem(TYPE *ary, int index, TYPE value) {
    ary[index] = value;
}
%}

TYPE *new_##NAME(int nelements);
void delete_##NAME(TYPE *ary);
TYPE NAME##_getitem(TYPE *ary, int index);
void NAME##_setitem(TYPE *ary, int index, TYPE value);

%enddef


/* -----------------------------------------------------------------------------
 * %array_class(TYPE,NAME)
 *
 * Generates a class wrapper around a C array.  The class has the following
 * interface:
 *
 *          struct NAME {
 *              NAME(int nelements);
 *             ~NAME();
 *              TYPE getitem(int index);
 *              void setitem(int index, TYPE value);
 *              TYPE * cast();
 *              static NAME *frompointer(TYPE *t);
  *         }
 *
 * ----------------------------------------------------------------------------- */

%define %array_class_old(TYPE,NAME)
%inline %{
#include <mteng.h>
%}
%{
typedef TYPE NAME;
%}
typedef struct {
  /* Put language specific enhancements here */
} NAME;

/*%pythonappend NAME::NAME(int nelements) %{ 
    self.count_ = nelements
%}*/

%extend NAME {

NAME(int nelements) {
  return (TYPE *) GAUSS_Malloc(nelements * sizeof(TYPE));
}

~NAME() {
//  GAUSS_Free(self);
}

TYPE getitem(int index) {
  return self[index];
}
void setitem(int index, TYPE value) {
  self[index] = value;
}
TYPE * cast() {
  return self;
}

static NAME *frompointer(TYPE *t) {
  return (NAME *) t;
}

/*%pythoncode %{
    def __len__(self):
        return self.count_
%}*/

};

%types(NAME = TYPE);

%enddef

