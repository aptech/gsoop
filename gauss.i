%module(directors="1") ge
%feature("director") IGEProgramOutput;
%feature("director") IGEProgramFlushOutput;
%feature("director") IGEProgramInputString;
%feature("director") IGEProgramInputChar;
%feature("director") IGEProgramInputCheck;
%include "std_string.i"
%include "std_vector.i"
%include "std_pair.i"
%include "typemaps.i"
%include "factory.i"

#ifdef SWIGPYTHON
%include "pyabc.i"
#endif

#ifdef SWIGWIN
%include "windows.i"
#endif

namespace std {
    %template(DoubleVector) vector<double>;
    %template(DoubleDoubleVector) vector<vector<double> >;
    %template(FloatVector) vector<float>;
    %template(IntVector) vector<int>;
    %template(StringVector) vector<string>;
    %template(StringStringVector) vector<vector<string> >;
}

#ifdef SWIGCSHARP
%include "arrays_csharp.i"
%apply double INPUT[] {const double *data}
%apply double INPUT[] {const double *imag_data}
%apply int INPUT[] {int *orders}
#endif

/*%rename(GESymType) GESymTypeNS;*/

%{
 /* Includes the header in the wrapper code */
 #include "src/gauss.h"
 #include "src/gesymbol.h"
 #include "src/gearray.h"
 #include "src/gematrix.h"
 #include "src/gestringarray.h"
 #include "src/geworkspace.h"
 #include "src/workspacemanager.h"
 #include "src/gefuncwrapper.h"
 #include "src/gesymtype.h"
%}

#ifdef SWIGCSHARP
/*
public int this[int key]
{
    get
    {
        return GetValue(key);
    }
    set
    {
        SetValue(key,value);
    }
}
*/
#endif

%newobject GAUSS::getSymbol;
%factory(GESymbol *GAUSS::getSymbol, GEMatrix, GEArray, GEStringArray);

#ifndef SWIGPHP
%newobject GAUSS::getMatrixDirect;
%newobject GAUSS::getMatrix;
%newobject GAUSS::getMatrixAndClear;
%newobject GAUSS::getArray;
%newobject GAUSS::getArrayAndClear;
%newobject GAUSS::getStringArray;
%newobject GEArray::getPlane;
%newobject GAUSS::loadWorkspace;
/*%newobject GAUSS::createWorkspace;*/
#endif
%delobject GAUSS::destroyWorkspace;

/* Start Python only*/
#ifdef SWIGPYTHON

%extend GESymbol {
    string __str__() {
        return $self->toString();
    }

    int __len__() {
        return $self->size();
    }
};

%pythoncode %{
class GEIterator:
    def __init__(self, data):
        self.index = 0
        self.data = data

    def __iter__(self):
        return self

    def next(self):
        if self.index >= len(self.data):
            raise StopIteration;
        ret = self.data[self.index]
        self.index += 1
        return ret

    __next__ = next
%}

%define ARRAYHELPER(type,name)
%extend name {
    type __getitem__(int i)
    {return $self->getElement(i);}
    void __setitem__(int i,type v)
    {$self->setElement(v, i);}

  %pythoncode %{
    def __iter__(self):
        return GEIterator(self)
  %}
}

%enddef

ARRAYHELPER(double, GEMatrix)
ARRAYHELPER(string, GEStringArray)

%rename(__getitem__) doubleArray::getitem;
%rename(__setitem__) doubleArray::setitem;
%rename(__len__) doubleArray::size;

%extend doubleArray {
%pythoncode %{
    def __iter__(self):
        return GEIterator(self)
%}
}

%factory(GESymbol *GAUSS::__getitem__, GEMatrix, GEArray, GEStringArray);
%extend GAUSS {
    GESymbol* __getitem__(char *name)
    {return $self->getSymbol(std::string(name));}
    void __setitem__(char *name,GESymbol* v)
    {$self->_setSymbol(v, std::string(name));}
    void __setitem__(char *name,doubleArray* v)
    {$self->_setSymbol(v, std::string(name));}
}

%exception __getitem__ {
   try {
      $action
   } catch (std::out_of_range &e) {
      PyErr_SetString(PyExc_IndexError, const_cast<char*>(e.what()));
      return NULL;
   }
}

/* Automatically release ownership of callback classes*/
%define CB_THISOWN(name,own)
%pythonappend name ## () {
    self.thisown = own
}
%enddef

/*
CB_THISOWN(IGEProgramOutput, 0)
CB_THISOWN(IGEProgramFlushOutput, 0)
CB_THISOWN(IGEProgramInputString, 0)
CB_THISOWN(IGEProgramInputChar, 0)
CB_THISOWN(IGEProgramInputCheck, 0)
*/

#endif
/* End Python only*/

/* Start PHP Only*/
#ifdef SWIGPHP

%extend GESymbol {
    string __toString() {
        return $self->toString();
    }
};

%factory(GESymbol *GAUSS::offsetGet, GEMatrix, GEArray, GEStringArray);
%typemap("phpinterfaces") GAUSS "ArrayAccess";
%extend GAUSS {
    GESymbol* offsetGet(char *name)
    {return $self->getSymbol(std::string(name));}

    void offsetSet(char *name,GESymbol* v)
    {$self->_setSymbol(v, std::string(name));}

    void offsetSet(char *name,doubleArray* v)
    {$self->_setSymbol(v, std::string(name));}

    bool offsetExists(char *name) {
        int t = $self->getSymbolType(std::string(name));

        return (t == GESymType::SCALAR || t == GESymType::MATRIX ||
                t == GESymType::ARRAY_GAUSS || t == GESymType::STRING ||
                t == GESymType::STRING_ARRAY);
    }

    void offsetUnset(char *name) {
       //$self->setElement(defvalue, offset); 
    }
}

%define ARRAYHELPER(type,name,defvalue)
%typemap("phpinterfaces") name "ArrayAccess, Countable, Iterator";
%extend name {
    void offsetSet(int offset, type value) {
        $self->setElement(value, offset);
    }

    bool offsetExists(int offset) {
        return offset < $self->size();
    }

    void offsetUnset(int offset) {
       $self->setElement(defvalue, offset); 
    }

    type offsetGet(int offset) {
        return $self->getElement(offset);
    } 

    int count() {
        // blah blah
        return $self->size();
    }

    void rewind() {
        $self->position_ = 0;
    }

    type current() {
        return $self->getElement($self->position_);
    }

    int key() {
        return $self->position_;
    }

    void next() {
        ++$self->position_;
    }

    bool valid() {
        return $self->position_ >= 0 && $self->position_ < $self->size(); 
    }
}

%enddef

ARRAYHELPER(double, GEMatrix, 0.0)
ARRAYHELPER(string, GEStringArray, "")

%rename(offsetGet) doubleArray::getitem;
%rename(offsetSet) doubleArray::setitem;
%rename(count) doubleArray::size;
%typemap("phpinterfaces") doubleArray "ArrayAccess, Countable, Iterator";
%extend doubleArray {
    bool offsetExists(int offset) {
        return offset >= 0 && offset < $self->size();
    }

    void offsetUnset(int offset) {
       $self[offset] = 0.0;
    }

    void rewind() {
        $self->position_ = 0;
    }

    double current() {
        return $self->getitem($self->position_);
    }

    int key() {
        return $self->position_;
    }

    void next() {
        ++$self->position_;
    }

    bool valid() {
        return $self->position_ >= 0 && $self->position_ < $self->size(); 
    }
}

#ifdef SWIGPHP5
%typemap(directorin) std::string, string, const string&, const std::string & %{
   ZVAL_STRINGL($input, const_cast<char*>($1.data()), $1.size(), 0);
%}

%typecheck(SWIG_TYPECHECK_STRING_ARRAY)
    std::vector<std::string> *,
    std::vector<double> *,
    std::vector<int> {
    // Custom array check
    $1 = Z_TYPE_PP($input) == IS_ARRAY;
}

%typemap(in) std::vector<std::string> * %{
{
    zval *arr, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(*$input);
    array_count = zend_hash_num_elements(arr_hash);

    $1 = new $1_basetype(array_count);

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {

        zval temp, *str;
        int is_str = 1;

        if(Z_TYPE_PP(data) != IS_STRING) {
            temp = **data;
            zval_copy_ctor(&temp);
            convert_to_string(&temp);
            str = &temp;
            is_str = 0;
        } else {
            str = *data;
        }

        $1->at(pointer->h) = std::string(Z_STRVAL_P(str));

        if (!is_str)
            zval_dtor(&temp);
    }
}
%}

%typemap(in) std::vector<double> *, std::vector<int> * %{
{
    zval *arr, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(*$input);
    array_count = zend_hash_num_elements(arr_hash);

    $1 = new $1_basetype(array_count);

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {
        switch (Z_TYPE_PP(data)) {
        case IS_BOOL:
        case IS_LONG:
            $1->at(pointer->h) = Z_LVAL_PP(data);
            break;
        case IS_DOUBLE:
            $1->at(pointer->h) = Z_DVAL_PP(data);
            break;
        }
    }
}
%}

%typemap(in) std::vector<double>, std::vector<int> %{
{
    zval *arr, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(*$input);
    array_count = zend_hash_num_elements(arr_hash);

    $1.resize(array_count);

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {
        switch (Z_TYPE_PP(data)) {
        case IS_BOOL:
        case IS_LONG:
            $1.at(pointer->h) = Z_LVAL_PP(data);
            break;
        case IS_DOUBLE:
            $1.at(pointer->h) = Z_DVAL_PP(data);
            break;
        }
    }
}
%}

#else
/* PHP 7 */

%typecheck(SWIG_TYPECHECK_STRING_ARRAY)
    std::vector<std::string> *,
    std::vector<double> *,
    std::vector<int> {
    // Custom array check
    $1 = Z_TYPE_P(&$input) == IS_ARRAY;
}

%typemap(in) std::vector<std::string> * %{
{
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(&$input);
    array_count = zend_hash_num_elements(arr_hash);

    $1 = new $1_basetype(array_count);

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {

        zval temp, *str;
        int is_str = 1;

        if(Z_TYPE_P(data) != IS_STRING) {
            temp = *data;
            zval_copy_ctor(&temp);
            convert_to_string(&temp);
            str = &temp;
            is_str = 0;
        } else {
            str = data;
        }

        $1->at(pointer) = std::string(Z_STRVAL_P(str));

        if (!is_str)
            zval_dtor(&temp);
    }
}
%}

%typemap(in) std::vector<double> *, std::vector<int> * %{
{
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(&$input);
    array_count = zend_hash_num_elements(arr_hash);

    $1 = new $1_basetype(array_count);

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {
        switch (Z_TYPE_P(data)) {
        case IS_TRUE:
        case IS_FALSE:
        case IS_LONG:
            $1->at(pointer) = Z_LVAL_P(data);
            break;
        case IS_DOUBLE:
            $1->at(pointer) = Z_DVAL_P(data);
            break;
        }
    }
}
%}

%typemap(in) std::vector<double>, std::vector<int> %{
{
    zval *data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(&$input);
    array_count = zend_hash_num_elements(arr_hash);

    $1.resize(array_count);

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        (data = zend_hash_get_current_data_ex(arr_hash, &pointer)) != NULL;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {
        switch (Z_TYPE_P(data)) {
        case IS_TRUE:
        case IS_FALSE:
        case IS_LONG:
            $1.at(pointer) = Z_LVAL_P(data);
            break;
        case IS_DOUBLE:
            $1.at(pointer) = Z_DVAL_P(data);
            break;
        }
    }
}
%}

#endif

%typemap(out) std::vector<string> %{
{
    array_init(return_value);

    for (int i = 0; i < $1.size(); ++i) {
#ifdef SWIGPHP5
        add_index_string(return_value, i, $1.at(i).c_str(), 1);
#else
        add_index_string(return_value, i, $1.at(i).c_str());
#endif
    }
}
%}

%typemap(out) std::vector<double> %{
{
    array_init(return_value);

    for (int i = 0; i < $1.size(); ++i) {
        add_index_double(return_value, i, $1.at(i));
    }
}
%}

%typemap(out) std::vector<int> %{
{
    array_init(return_value);

    for (int i = 0; i < $1.size(); ++i) {
        add_index_long(return_value, i, $1.at(i));
    }
}
%}
#endif

/* End PHP Only*/

/* Not using this anymore
%include "gausscarrays.i"
%array_class(double, doubleArray);
*/

/* Ignore stub functions */
%ignore hookStubOutput;
%ignore hookStubError;
%ignore hookStubFlush;
%ignore hookStubInputString;
%ignore hookStubInputChar;
%ignore hookStubInputBlockingChar;
%ignore hookStubInputCheck;

%ignore GEStringArray::GEStringArray(StringArray_t*);
%ignore GEStringArray::Init(StringArray_t*);
%ignore GEStringArray::toInternal();
%ignore GEArray::GEArray(Array_t*);
%ignore GEArray::Init(Array_t*);
%ignore GEArray::toInternal();
%ignore GEMatrix::GEMatrix(Matrix_t*);
%ignore GEMatrix::GEMatrix(GAUSS_MatrixInfo_t*);
%ignore GEMatrix::toInternal();

/* Parse the header file to generate wrappers */
%include "src/gauss.h"
%include "src/gesymbol.h"
%include "src/gearray.h"
%include "src/gematrix.h"
%include "src/gestringarray.h"
%include "src/geworkspace.h"
%include "src/workspacemanager.h"
%include "src/gefuncwrapper.h"
%include "src/gesymtype.h"

