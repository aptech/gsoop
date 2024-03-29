#include "gestringarray.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <sstream>


GEStringArray::GEStringArray() : GESymbol(GESymType::STRING_ARRAY)
{
    this->setRows(1);
    this->setCols(1);
}

GEStringArray::GEStringArray(StringArray_t *sa) : GESymbol(GESymType::STRING_ARRAY) {
    fromStringArray(sa);
}

/**
 * Initialize a one-dimensional std::string array with a user specified std::vector. This assumes
 * that there are `data.size()` columns and 1 row.
 *
 * Example:
 *
__Python__
```py
sa = GEStringArray(["Open", "High", "Low"])
print sa
```
 *
__PHP__
```php
$sa = new GEStringArray(array("Open", "High", "Low"))
echo $sa;
```
 * results in output:
```
Open    High    Low
```
 *
 * @param data
 */
GEStringArray::GEStringArray(VECTOR_DATA(std::string) data) : GESymbol(GESymType::STRING_ARRAY) {
    setData(data, 1, VECTOR_VAR(data) size());
}

/**
 * Initialize a std::string array from a std::vector with dimensions _rows_ and _cols_.
 *
 * Example:
 *
__Python__
```py
sa = GEStringArray(["Open", "High", "Low", "Close"], 2, 2)
print sa
```
 *
__PHP__
```php
$sa = new GEStringArray(array("Open", "High", "Low", "Close"), 2, 2);
echo $sa;
```
 * results in output:
```
Open    High
Low     Close
```
 *
 * @param data        One-dimensional std::string data
 * @param rows        Row count
 * @param cols        Column count
 */
GEStringArray::GEStringArray(VECTOR_DATA(std::string) data, int rows, int cols) : GESymbol(GESymType::STRING_ARRAY) {
    setData(data, rows, cols);
}

/**
 * Return the std::string value at the _row_, _column_ index.
 *
 * Example:
 *
__Python__
```py
sa = GEStringArray(["foo", "bar", "baz"])
print str(sa.getElement(0, 1))
```
 *
__PHP__
```php
$sa = new GEStringArray(array("foo", "bar", "baz"));
echo $sa->getElement(0, 1);
```
 * results in the output:
```
bar
```
 *
 * @param row        Row index
 * @param col        Column index
 * @return        Value at specified index
 */
std::string GEStringArray::getElement(int row, int col) const {
    unsigned int index = row * this->getCols() + col;

    if (index >= this->data_.size() || row >= this->getRows() || col >= this->getCols())
        return std::string();

    return this->data_[index];
}

/**
 * Return the std::string value at the absolute position _index_.
 *
 * Example:
 *
__Python__
```py
sa = GEStringArray(["foo", "bar", "baz"])
print sa[-2]
```
 *
__PHP__
```php
$sa = new GEStringArray(array("foo", "bar", "baz"));
echo $sa->getElement(0, 1);
```
 * results in the output:
```
bar
```
 *
 * @param index        Index
 * @return        Value at specified index
 */
std::string GEStringArray::getElement(int index) const {
    if (index < 0)
        index += this->data_.size();

    if (index >= this->data_.size())
        return std::string();

    return this->data_[index];
}

/**
 * Return a collection copy of std::strings as a std::vector.
 *
 * Example:
 *
__Python__
```py
# Create a std::string array using GAUSS
ge.executeString("string sa = { one two three four, five six seven eight }")

# Retrieve the std::string array from the symbol table
sa = ge.getStringArray("sa")

print sa
print " ".join(sa.getData())
```
 *
__PHP__
```php
// Create a std::string array using GAUSS
$ge->executeString("string sa = { one two three four, five six seven eight };");

// Retrieve the std::string array from the symbol table
$sa = $ge->getStringArray("sa");

echo $sa . PHP_EOL;
echo implode(" ", $sa->getData()) . PHP_EOL;
```
 * resulting in the output:
```
ONE     TWO     THREE   FOUR
FIVE    SIX     SEVEN   EIGHT

ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT
```
 *
 * @return        std::string std::vector
 */
std::vector<std::string> GEStringArray::getData() const {
    return this->data_;
}

/**
  * Replace the contents of the std::string collection. The length of
  * _data_ should be equal to _rows_ * _cols_.
  *
  * @param rows     Rows
  * @param cols     Cols
  */
void GEStringArray::setData(VECTOR_DATA(std::string) data, int rows, int cols) {
#ifdef SWIGPHP
    data->swap(this->data_);
    delete data;
#else
    this->data_ = data;
#endif
    if (rows * cols > this->data_.size()) {
        rows = this->data_.size();
        cols = 1;
    }

    this->setRows(rows);
    this->setCols(cols);
}

/**
 * Set std::string value to _str_ at _row_, _col_ element.
 *
 * Example:
 *
__Python__
```py
sa = GEStringArray(["foo", "bar", "baz"])
sa.setElement("foo", 0, 1)
print sa
```
 *
__PHP__
```php
$sa = new GEStringArray(array("foo", "bar", "baz"));
$sa->setElement("foo", 0, 1);
echo $sa->toString();
```
 * results in the output:
```
foo        foo        baz
```
 *
 * @param row      Row
 * @param col      Col
 */
bool GEStringArray::setElement(const std::string &str, int row, int col) {
    unsigned int index = row * this->getCols() + col;

    if (index >= data_.size())
        return false;

    data_[index] = str;

    return true;
}

/**
 * Set std::string value to _str_ at absolute position _index_.
 *
 * Example:
 *
__Python__
```py
sa = GEStringArray(["foo", "bar", "baz"])
sa[1] = "foo"
print sa
```
 *
__PHP__
```php
$sa = new GEStringArray(array("foo", "bar", "baz"));
$sa->setElement("foo", 0, 1);
echo $sa->toString();
```
 * results in the output:
```
foo        foo        baz
```
 *
 * @param index      Index
 */
bool GEStringArray::setElement(const std::string &str, int index) {
    if (fabs(index) >= data_.size())
        return false;

    if (index < 0)
        index += this->data_.size();

    this->data_[index] = str;

    return true;
}

bool GEStringArray::fromStringArray(StringArray_t *sa) {
    if (sa == nullptr)
        return false;

    int rows = sa->rows;
    int cols = sa->cols;

    this->setRows(rows);
    this->setCols(cols);

    if (rows == 0 || cols == 0)
        return false;

    int element_count = rows * cols;

    this->data_.resize(element_count);

    StringElement_t *sep = sa->table;
    const char *buffer_start = (const char*)(sa->table + element_count);

    for (int i = 0; i < element_count; ++i, ++sep)
        this->data_[i] = std::string(buffer_start + sep->offset);

    GAUSS_Free(sa->table);
    GAUSS_Free(sa);

    return true;
}

std::string GEStringArray::toString() const {
    std::stringstream s;

    int rows = getRows();
    int cols = getCols();

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            s << getElement(i, j);

            if (j < cols - 1)
                s << '\t';
        }

        if (i < rows - 1)
            s << '\n';
    }

    return s.str();
}

StringArray_t* GEStringArray::toInternal() {
    if (!size())
        return nullptr;

    StringArray_t *sa;
    StringElement_t *stable;
    StringElement_t *sep;
    size_t elem;
    size_t strsize, sasize;

    sa = (StringArray_t *)malloc(sizeof(StringArray_t));

    if (sa == nullptr)
        return nullptr;

    elem = size();
    sa->baseoffset = (size_t)(elem*sizeof(StringElement_t));

    stable = (StringElement_t *)malloc(sa->baseoffset);

    if (stable == nullptr)
    {
        free(sa);
        return nullptr;
    }

    sep = stable;
    strsize = 0;

    for (int i = 0; i < elem; ++i, ++sep) {
        sep->offset = strsize;
        sep->length = data_.at(i).length() + 1;
        strsize += sep->length;
    }

    sasize = getsize(strsize + sa->baseoffset, 1, 1);
    stable = (StringElement_t*)realloc(stable, sasize * sizeof(double));

    if (stable == nullptr)
    {
        free(sa);
        free(stable);
        return nullptr;
    }

    sep = stable;

    for (int i = 0; i < elem; ++i, ++sep)
        memcpy((char *)stable + sa->baseoffset + sep->offset, data_.at(i).c_str(), sep->length);

    sa->size = sasize;
    sa->rows = getRows();
    sa->cols = getCols();
    sa->table = stable;
    sa->freeable = TRUE;

    return sa;
}
