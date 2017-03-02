%module(directors="1") ge
%feature("director") IGEProgramOutput;
%feature("director") IGEProgramFlushOutput;
%feature("director") IGEProgramInputString;
%feature("director") IGEProgramInputChar;
%feature("director") IGEProgramInputCheck;
%include "std_string.i"
%include "std_vector.i"
%include "typemaps.i"

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
%apply double INPUT[] {double *data}
%apply double INPUT[] {double *imag_data}
%apply int INPUT[] {int *orders}
/* allow partial c# classes */
/*%typemap(csclassmodifiers) SWIGTYPE "public partial class"*/
/*%extend StringVector {
    public string[] toArray()
};*/
/*
namespace std {
%typemap(csclassmodifiers) StringVector "public partial class"
%typemap(cscode) StringVector %{
    // cast from C# string array
    public static implicit operator StringVector(string[] inVal) {
        var outVal= new StringVector();
        foreach (string element in inVal) {
            outVal.Add(element);
        }
        return outVal;
    }

    // cast to C# string array
    public static implicit operator string[](StringVector inVal) {
        var outVal= new string[inVal.Count];
        inVal.CopyTo(outVal);
        return outVal;
    }
%}
}
*/

#endif

/*%rename(GESymType) GESymTypeNS;*/
#define GAUSS_EXPORT 

%{
 /* Includes the header in the wrapper code */
 #include "src/gauss.h"
 #include "src/gesymbol.h"
 #include "src/gearray.h"
 #include "src/gematrix.h"
 #include "src/gestring.h"
 #include "src/gestringarray.h"
 #include "src/geworkspace.h"
 #include "src/workspacemanager.h"
 #include "src/gefuncwrapper.h"
 #include "src/gesymtype.h"
%}

// Start Python only
#ifdef SWIGPYTHON

%extend GESymbol {
    string __str__() {
        return $self->toString();
    }
};

// Automatically release ownership of callback classes
/*
%pythonappend IGEProgramOutput() {
    self.thisown = 0
};

%pythonappend IGEProgramFlushOutput() {
    self.thisown = 0
};

%pythonappend IGEProgramInputString() {
    self.thisown = 0
};

%pythonappend IGEProgramInputChar() {
    self.thisown = 0
};

%pythonappend IGEProgramInputCheck() {
    self.thisown = 0
};
*/

#endif
// End Python only

// Start PHP Only

#ifdef SWIGPHP

%extend GESymbol {
    string __toString() {
        return $self->toString();
    }
};

%typemap(directorin) std::string, string, const string&, const std::string & %{
   ZVAL_STRINGL($input, const_cast<char*>($1.data()), $1.size(), 0);
%}

%typecheck(SWIG_TYPECHECK_STRING_ARRAY)
    std::vector<std::string>,
    std::vector<double>,
    std::vector<int> {
    // Custom array check
    $1 = Z_TYPE_PP($input) == IS_ARRAY;
}

%typemap(in) std::vector<std::string> %{
{
    zval *arr, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    int array_count;

    arr_hash = Z_ARRVAL_P(*$input);
    array_count = zend_hash_num_elements(arr_hash);

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

        $1.push_back(std::string(Z_STRVAL_P(str)));

        if (!is_str)
            zval_dtor(&temp);
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

    for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
        zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS;
        zend_hash_move_forward_ex(arr_hash, &pointer))
    {
        switch (Z_TYPE_PP(data)) {
        case IS_BOOL:
        case IS_LONG:
            $1.push_back(Z_LVAL_PP(data));
            break;
        case IS_DOUBLE:
            $1.push_back(Z_DVAL_PP(data));
            break;
        }
    }
}
%}

%typemap(out) std::vector<string> %{
{
    array_init(return_value);

    for (int i = 0; i < $1.size(); ++i) {
        add_index_string(return_value, i, $1.at(i).c_str(), 1);
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

// End PHP Only

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
%ignore GEArray::GEArray(Array_t*);
%ignore GEArray::Init(Array_t*);
%ignore GEMatrix::GEMatrix(Matrix_t*);
%ignore GEMatrix::Init(Matrix_t*);
%ignore GEString::GEString(String_t*);
%ignore GEString::Init(String_t*);

/* Parse the header file to generate wrappers */
%include "src/gauss.h"
%include "src/gesymbol.h"
%include "src/gearray.h"
%include "src/gematrix.h"
%include "src/gestring.h"
%include "src/gestringarray.h"
%include "src/geworkspace.h"
%include "src/workspacemanager.h"
%include "src/gefuncwrapper.h"
%include "src/gesymtype.h"

