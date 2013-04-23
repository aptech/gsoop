#include "gestringarray.h"
#include <iostream>
#include <cstring>
#include <sstream>
using namespace std;

GEStringArray::GEStringArray()
{
    this->setRows(1);
    this->setCols(1);
}

GEStringArray::GEStringArray(StringArray_t *sa) {
    fromStringArray(sa);
}

/**
 * Initialize a one-dimensional string array with a user specified vector. This assumes
 * that there are `data.size()` columns and 1 row.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
sa = GEStringArray(["Open", "High", "Low"])
print sa
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$sa = new GEStringArray(array("Open", "High", "Low"))
echo $sa;
 * ~~~
 * results in output:
 * ~~~
Open    High    Low
 * ~~~
 *
 * @param data
 */
GEStringArray::GEStringArray(vector<string> data) {
    setData(data, 1, data.size());
}

/**
 * Initialize a string array from a vector with dimensions _rows_ and _cols_.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
sa = GEStringArray(["Open", "High", "Low", "Close"], 2, 2)
print sa
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$sa = new GEStringArray(array("Open", "High", "Low", "Close"), 2, 2);
echo $sa;
 * ~~~
 * results in output:
 * ~~~
Open    High
Low     Close
 * ~~~
 *
 * @param data        One-dimensional string data
 * @param rows        Row count
 * @param cols        Column count
 */
GEStringArray::GEStringArray(vector<string> data, int rows, int cols) {
    setData(data, rows, cols);
}

/**
 * Return the string value at the _row_, _column_ index.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
sa = GEStringArray(["foo", "bar", "baz"])
print str(sa.getElement(0, 1))
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$sa = new GEStringArray(array("foo", "bar", "baz"));
echo $sa->getElement(0, 1);
 * ~~~
 * results in the output:
 * ~~~
bar
 * ~~~
 *
 * @param row        Row index
 * @param col        Column index
 * @return        Value at specified index
 */
string GEStringArray::getElement(int row, int col) {
    unsigned int index = row * this->getCols() + col;

    if (index >= data_.size() || row >= this->getRows() || col >= this->getCols())
        return string();

    return string(this->data_[index]);
}

/**
 * Return a collection copy of strings as a vector.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
# Create a string array using GAUSS
ge.executeString("string sa = { one two three four, five six seven eight }")

# Retrieve the string array from the symbol table
sa = ge.getStringArray("sa")

print sa
print " ".join(sa.getData())
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
// Create a string array using GAUSS
$ge->executeString("string sa = { one two three four, five six seven eight };");

// Retrieve the string array from the symbol table
$sa = $ge->getStringArray("sa");

echo $sa . PHP_EOL;
echo implode(" ", $sa->getData()) . PHP_EOL;
 * ~~~
 * resulting in the output:
 * ~~~
ONE     TWO     THREE   FOUR
FIVE    SIX     SEVEN   EIGHT

ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT
 * ~~~
 *
 * @return        string vector
 */
vector<string> GEStringArray::getData() {
    return vector<string>(data_);
}

/**
  * Replace the contents of the string collection. The length of
  * _data_ should be equal to _rows_ * _cols_.
  *
  * @param rows     Rows
  * @param cols     Cols
  */
void GEStringArray::setData(vector<string> data, int rows, int cols) {
    if (rows * cols > data.size()) {
        rows = data.size();
        cols = 1;
    }

    this->data_ = data;
    this->setRows(rows);
    this->setCols(cols);
}

/**
 * Set string value to _str_ at _row_, _col_ element.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
sa = GEStringArray(["foo", "bar", "baz"])
sa.setElement("foo", 0, 1)
print sa
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$sa = new GEStringArray(array("foo", "bar", "baz"));
$sa->setElement("foo", 0, 1);
echo $sa->toString();
 * ~~~
 * results in the output:
 * ~~~
foo        foo        baz
 * ~~~
 *
 * @param row      Row
 * @param col      Col
 */
bool GEStringArray::setElement(string str, int row, int col) {
    unsigned int index = row * this->getCols() + col;

    if (index >= data_.size())
        return false;

    data_[index] = str;

    return true;
}

bool GEStringArray::fromStringArray(StringArray_t *sa) {
    if (sa == NULL)
        return false;

    int rows = sa->rows;
    int cols = sa->cols;

    this->setRows(rows);
    this->setCols(cols);

    if (rows == 0 || cols == 0)
        return false;

    int element_count = rows * cols;

    this->data_.clear();
    this->data_.resize(element_count);

    StringElement_t *current_ptr = 0;
    StringElement_t *base_ptr = sa->table;
    size_t base_offset = sa->baseoffset;

    char *ch_ptr;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            current_ptr = base_ptr + (i * cols + j);
            ch_ptr = (char*)((char*)base_ptr + base_offset + current_ptr->offset);

            string val = string(ch_ptr);
            this->data_[i * cols + j] = val;
        }
    }

    GAUSS_Free(sa->table);
    GAUSS_Free(sa);

    return true;
}

string GEStringArray::toString() {
    stringstream s;

    int rows = getRows();
    int cols = getCols();

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            s << getElement(i, j) << '\t';
        }

        if (i < rows - 1)
            s << endl;
    }

    return s.str();
}
