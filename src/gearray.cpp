#include "gearray.h"
#include "gematrix.h"
#include <cstring>
#include <sstream>


/**
 * Initialize a one dimensional array with a single element set to zero.
 */
GEArray::GEArray() : GESymbol(GESymType::ARRAY_GAUSS)
{
    clear();
}

GEArray::GEArray(Array_t *array) : GESymbol(GESymType::ARRAY_GAUSS) {
    Init(array);
}

/**
 * Initialize an array with the orders specified. You need to fill in the array data
 * from lowest dimension to highest dimension. If the data is complex, it should contain
 * all the real values in the first half of the array, followed by all the imaginary
 * values in the second half of the array.
 *
 * Example:
 *
__Python__
```py
orders = [4, 3, 2]
data = range(1, 49)
a = GEArray(orders, data, True)     # Indicate data is complex
```
 *
__PHP__
```php
$orders = array(4, 3, 2);
$data = range(1.0, 48.0);
$a = new GEArray($orders, $data, true); // Indicate data is complex
```
 *
 * will create a complex array with the elements filled in as follows:
 *
```
[1,1,1] =  1.0 + 25.0i,  [1,1,2] =  2.0 + 26.0i,
[1,2,1] =  3.0 + 27.0i,  [1,2,2] =  4.0 + 28.0i,
[1,3,1] =  5.0 + 29.0i,  [1,3,2] =  6.0 + 30.0i,

[2,1,1] =  7.0 + 31.0i,  [2,1,2] =  8.0 + 32.0i,
[2,2,1] =  9.0 + 33.0i,  [2,2,2] = 10.0 + 34.0i,
[2,3,1] = 11.0 + 35.0i,  [2,3,2] = 12.0 + 36.0i,

...
```
 *
 * @param orders        Dimension orders from highest to lowest left-to-right.
 * @param data                Data in one dimensional format
 * @param complex        True if array is complex, false otherwise.
 */
GEArray::GEArray(std::vector<int> orders, VECTOR_DATA(double) data, bool complex) : GESymbol(GESymType::ARRAY_GAUSS) {
    Init(&orders.front(), orders.size(), &VECTOR_VAR(data) front(), VECTOR_VAR(data) size(), complex);

    VECTOR_VAR_DELETE_CHECK(data);
}

GEArray::GEArray(const int *orders, int orders_len, const double *data, int data_len, bool complex) : GESymbol(GESymType::ARRAY_GAUSS) {
    Init(orders, orders_len, data, data_len, complex);
}

void GEArray::Init(const int *orders, int orders_len, const double *data, int data_len, bool complex) {
    this->dims_ = orders_len;
    this->setComplex(complex);
    this->num_elements_ = 1;

    for (int i = 0; i < dims_; ++i)
        this->num_elements_ *= orders[i];

    int realElements = num_elements_;

    if (complex)
        realElements *= 2;

    this->data_.resize(realElements + orders_len);

    for (int i = 0; i < orders_len; ++i)
        this->data_[i] = (double)orders[i];

    memcpy(this->data_.data() + orders_len, data, realElements * sizeof(double));

    if (this->dims_ > 1) {
        this->setRows(this->data_[this->dims_ - 2]);
        this->setCols(this->data_[this->dims_ - 1]);
    }
}

/** \internal */
bool GEArray::Init(Array_t *array) {
    if (!array)
        return false;

    this->setComplex(static_cast<bool>(array->complex));
    this->dims_ = array->dims;
    this->num_elements_ = 1; // Fix for getArrayAndClear not setting nelems.

    for (int i = 0; i < this->dims_; ++i)
        this->num_elements_ *= (size_t)array->adata[i];

    int realElements = totalElements();

    this->data_.resize(realElements + this->dims_);

    for (int i = 0; i < this->dims_; ++i) {
        int order = array->adata[i];

        this->data_[i] = (double)order;

        if (i == this->dims_ - 2)
            this->setRows(order);
        else if (i == this->dims_ - 1)
            this->setCols(order);
    }

    memcpy(this->data_.data() + this->dims_, array->adata + this->dims_, realElements * sizeof(double));

    GAUSS_Free(array->adata);
    GAUSS_Free(array);

    return true;
}

std::string GEArray::toString() const {
    std::stringstream ss;

    bool complex = isComplex();
    int rows = getRows();
    int cols = getCols();
    int elements_per_plane = rows * cols;
    int total_elements = num_elements_;
    int plane_count = total_elements / elements_per_plane;

    const double *base = this->data_.data() + this->dims_;

    int index = 0;

    int dimslength = dims_ - 2;
    std::vector<int> dims(dimslength);

    // init array
    for (int i = 0; i < dimslength; ++i)
        dims[i] = 1;

    for (int i = 0; i < plane_count; ++i) {

        ss << "Plane [";

        for (int d = 0; d < dimslength; ++d)
            ss << dims[d] << ",";

        for (int d = dimslength - 1; d >= 0; --d) {
            if (dims[d] < data_[d]) {
                dims[d]++;

                for (int e = d + 1; e < dimslength; ++e)
                    dims[e] = 1;

                break;
            }
        }

        ss << ".,.]" << std::endl << std::endl;

        base = this->data_.data() + this->dims_ + (i * elements_per_plane);

        for (int j = 0; j < rows; ++j) {
            for (int k = 0; k < cols; ++k) {
                index = j * cols + k;

                ss << "\t" << *(base + index);

                if (complex)
                    ss << " + " << *(base + total_elements + index) << "i";

                if (k != (cols - 1))
                    ss << "\t";
            }

            ss << std::endl;
        }

        if (i < plane_count - 1)
            ss << std::endl;
    }

    return ss.str();
}

/**
 * Retrieve a 2-dimensional slice from an array. The indices array will contain N-2 normal indices into the array,
 * positioning to the first element of the plane of interest, and two indices set to 0, indicating the dimensions of
 * interest. The plane of elements across those 2 dimensions will be returned, the lower dimension becoming columns
 * and the higher dimension becoming rows.
 * This function returns either the real or the imaginary part of the data, based on the <i>imag</i> argument.
 *
 * If you need to extract more than two dimensions of data, you can use the GAUSS indexing (square bracket)
 * operators or the GAUSS.getArray(std::string) function. See the N-Dimensional Arrays and Working With Arrays chapters of the GAUSS
 * User Guide.
 *
 * Example:
 *
__Python__
```py
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")

# p is now a GEMatrix object
p = a.getPlane([0, 2, 0])

for i in range(0, p.getRows()):
    for j in range(0, p.getCols()):
        print str(p.getElement(i, j)) + "\t",

    print
```
 *
__PHP__
```php
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");

// $p is now a GEMatrix object
$p = $a->getPlane(array(0, 2, 0));

for ($i = 0; $i < $p->getRows(); ++$i) {
    for ($j = 0; $j < $p->getCols(); ++$j) {
        echo $p->getElement($i, $j) . "\t";
    }

    echo PHP_EOL;
}
```
 * will result in the output:
```
5        6        7        8
17        18        19        20
```
 * extracted from the values of a:
```
Plane [1,.,.]

       1.0000000        2.0000000        3.0000000        4.0000000
       5.0000000        6.0000000        7.0000000        8.0000000
       9.0000000        10.000000        11.000000        12.000000

Plane [2,.,.]

       13.000000        14.000000        15.000000        16.000000
       17.000000        18.000000        19.000000        20.000000
       21.000000        22.000000        23.000000        24.000000
```
 *
 * @param indices        Indices indicating the plane to retrieve, with 0's representing the dimensions of interest
 * @param imag        Whether to return imaginary data instead of real data.
 * @return        2 dimensional array slice.
 */

GEMatrix* GEArray::getPlane(std::vector<int> indices, bool imag) const {
    bool complex = isComplex();
    int offset = (imag && complex ? this->num_elements_ : 0);

    int zero_count = 0;

    int odoi[2];
    int jdoi[2];

    int product = 1;
    int jumpoff = 0;

    const int HIGH = 0;
    const int LOW = 1;

    for (int i = dims_ - 1; i >= 0; --i) {
        int index = indices[i];

        if (index != 0) {
            // check for out of range
            if (index < 1 || index > this->data_[i])
                return nullptr;

            jumpoff += (index - 1) * product;
        } else {
            zero_count++;

            if (zero_count == 1) {
                odoi[LOW] = this->data_[i];
                jdoi[LOW] = product;
            } else {
                odoi[HIGH] = this->data_[i];
                jdoi[HIGH] = product;
            }
        }

        product *= this->data_[i];
    }

    // 2 dimensions of interest were not specified
    if (zero_count != 2)
        return nullptr;

    int rows = odoi[HIGH];
    int cols = odoi[LOW];

    const double *base = this->data_.data() + this->dims_;

    VECTOR_DATA_INIT(plane, double, rows * cols);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            // If it's not complex but they want imaginary data, returns 0's.
            if (!complex && imag)
                VECTOR_VAR(plane) at(i * cols + j) = 0;
            else {
                int index = jumpoff + (jdoi[HIGH] * i) + (jdoi[LOW] * j) + offset;

                // index out of range
                if (index > num_elements_) {
                    VECTOR_VAR_DELETE_CHECK(plane);
                    return nullptr;
                }

                VECTOR_VAR(plane) at(i * cols + j) = base[index];
            }
        }
    }

    return new GEMatrix(plane, rows, cols, imag);
}

/**
 * Retrieve a one dimensional std::vector from an array. The dimension of interest must be specified with a 0.
 * This will get a std::vector of elements across the dimension. This function only returns either the real
 * or the imaginary part of the data based on the _imag_ argument.
 *
 * Example:
 *
__Python__
```py
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")

# p is now a GEMatrix object
v = a.getVector([0, 3, 4])

print ", ".join([str(n) for n in v])
```
 *
__PHP__
```php
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");

// $p is now a GEMatrix object
$v = $a->getVector(array(0, 3, 4));

echo implode(", ", $v) . PHP_EOL;
```
 * will result in the output:
```
12, 24
```
 * extracted from the value of a:
```
Plane [1,.,.]

       1.0000000        2.0000000        3.0000000        4.0000000
       5.0000000        6.0000000        7.0000000        8.0000000
       9.0000000        10.000000        11.000000        12.000000

Plane [2,.,.]

       13.000000        14.000000        15.000000        16.000000
       17.000000        18.000000        19.000000        20.000000
       21.000000        22.000000        23.000000        24.000000
```
 *
 * @param indices        Indices that you would like to retrieve, with a 0 representing the dimension of interest
 * @param imag        Whether to return imaginary data instead of real data.
 * @return        Vector of data
 */
std::vector<double> GEArray::getVector(std::vector<int> indices, bool imag) const {
    bool complex = isComplex();

    if (indices.empty() || (imag && !complex))
        return std::vector<double>();

    int offset = imag ? this->num_elements_ / 2 : 0;
    int elements = totalElements();

    int zero_count = 0;

    int odoi = 0;
    int jdoi = 0;

    int product = 1;
    int jumpoff = 0;

    for (int i = dims_ - 1; i >= 0; --i) {
        int index = indices[i];

        if (index != 0) {
            // check for out of range
            if (index < 1 || index > this->data_[i])
                return std::vector<double>();

            jumpoff += (index - 1) * product;
        } else {
            zero_count++;

            if (zero_count == 1) {
                odoi = this->data_[i];
                jdoi = product;
            }
        }

        product *= this->data_[i];
    }

    // 1 dimension of interest were not specified
    if (zero_count != 1)
        return std::vector<double>();

    const double *base = this->data_.data() + this->dims_;
    std::vector<double> result(odoi);

    for (int i = 0; i < odoi; ++i) {
        int index = jumpoff + (jdoi * i) + offset;

        // index out of range
        if (index > elements)
            return std::vector<double>();

        result[i] = base[index];
    }

    return result;
}

/**
 * Retrieve an element from an array. The indices array will contain N normal indices into the array,
 * indicating the element you want. This function only returns either the real
 * or the imaginary part of the data based on the _imag_ argument
 *
 * Example:
 *
__Python__
```py
# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")
print str(a.getElement([1, 1, 2])) # Second element on first row of first plane
print str(a.getElement([1, 2, 1])) # First element on second row of first plane
print str(a.getElement([2, 1, 1])) # First element on first row of second plane
```
 *
__PHP__
```php
// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");
echo $a->getElement(array(1, 1, 2)) . PHP_EOL; // Second element on first row of first plane
echo $a->getElement(array(1, 2, 1)) . PHP_EOL; // First element on second row of first plane
echo $a->getElement(array(2, 1, 1)) . PHP_EOL; // First element on first row of second plane
```
 * will result in the output:
```
2
5
13
```
 * extracted from the values of a:
```
Plane [1,.,.]

       1.0000000        2.0000000        3.0000000        4.0000000
       5.0000000        6.0000000        7.0000000        8.0000000
       9.0000000        10.000000        11.000000        12.000000

Plane [2,.,.]

       13.000000        14.000000        15.000000        16.000000
       17.000000        18.000000        19.000000        20.000000
       21.000000        22.000000        23.000000        24.000000
```
 *
 * @param indices        Indices indicating the element to retrieve
 * @param imag        Whether to return imaginary data instead of real data.
 * @return        double precision element
 */
double GEArray::getElement(std::vector<int> indices, bool imag) const {
    if (indices.empty() || (imag && !isComplex()))
        return 0.0;

    int offset = imag ? this->num_elements_ / 2 : 0;

    int product = 1;
    int jumpoff = 0;

    for (int i = dims_ - 1; i >= 0; --i) {
        int index = indices[i];

        if (index < 1 || index > this->data_[i])
            return 0;

        jumpoff += (index - 1) * product;

        product *= this->data_[i];
    }

    int index = jumpoff + offset;

    if (index > totalElements())
        return 0.0;

    return this->data_[index + this->dims_];
}

/**
 * Set an element in an array. The indices array will contain N normal indices into the array,
 * indicating the element you want to set the new value of. This function sets either the real
 * or the imaginary part of the data based on the _imag_ argument.
 *
 * Example:
 *
__Python__
```py
# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")
a.setElement(0, [1, 1, 2]) # Second element on first row of first plane
a.setElement(0, [1, 2, 1]) # First element on second row of first plane
a.setElement(0, [2, 1, 1]) # First element on first row of second plane
ge.setSymbol(a, "a")
ge.executeString("print a")
```
 *
__PHP__
```php
// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");
$a->setElement(0, array(1, 1, 2)); // Second element on first row of first plane
$a->setElement(0, array(1, 2, 1)); // First element on second row of first plane
$a->setElement(0, array(2, 1, 1)); // First element on first row of second plane
$ge->setSymbol($a, "a");
$ge->executeString("print a;");
```
 * will result in the output:
```
Plane [1,.,.]

       1.0000000        0.0000000        3.0000000        4.0000000
       0.0000000        6.0000000        7.0000000        8.0000000
       9.0000000        10.000000        11.000000        12.000000

Plane [2,.,.]

        0.000000        14.000000        15.000000        16.000000
       17.000000        18.000000        19.000000        20.000000
       21.000000        22.000000        23.000000        24.000000
```
 * extracted from the values of a:
```
Plane [1,.,.]

       1.0000000        2.0000000        3.0000000        4.0000000
       5.0000000        6.0000000        7.0000000        8.0000000
       9.0000000        10.000000        11.000000        12.000000

Plane [2,.,.]

       13.000000        14.000000        15.000000        16.000000
       17.000000        18.000000        19.000000        20.000000
       21.000000        22.000000        23.000000        24.000000
```
 *
 * @param value                Double precision value to set at element index
 * @param indices        Indices indicating the element to set
 * @param imag                Whether to set imaginary data instead of real data.
 */
bool GEArray::setElement(double value, std::vector<int> indices, bool imag) {
    if (indices.empty() || (imag && !isComplex()))
        return false;

    int offset = imag ? this->num_elements_ : 0;

    int product = 1;
    int jumpoff = 0;

    for (int i = dims_ - 1; i >= 0; --i) {
        int index = indices[i];

        if (index < 1 || index > this->data_[i])
            return false;

        jumpoff += (index - 1) * product;

        product *= this->data_[i];
    }

    int index = jumpoff + offset;

    if (index > totalElements())
        return false;

    this->data_[index + this->dims_] = value;

    return true;
}

/**
 * Retrieve a copy of underlying numeric std::vector. Imaginary data can also
 * be queried by supplying the _imag_ argument, thus appending it to the
 * end of the real data. Omitting or specifying the _imag_ argument as `false`
 * will return only real data. If you wish to only view the imaginary data,
 * there is a getImagData() method for convenience.
 *
 * Example:
 *
__Python__
```py
# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")
print ", ".join(str(n) for n in a.getData())
```
 *
__PHP__
```php
// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");
echo implode(", ", $a->getData()) . PHP_EOL;
```
 * will result in the output:
```
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24
```
 *
 * @param imag  Whether to append imaginary data to end of real data
 * @return Array data as a one-dimensional std::vector
 */
std::vector<double> GEArray::getData(bool imag) const {
    if (imag && !isComplex())
        return std::vector<double>();

    int elements = imag ? totalElements() : this->num_elements_;

    std::vector<double> ret(elements);

    memcpy(ret.data(), this->data_.data() + this->dims_, elements * sizeof(double));

    return ret;
}

/**
 * Retrieve underlying imaginary data storage. This will be stored in lowest to highest dimension.
 *
 * Example:
 *
__Python__
```py
# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
ge.executeString("b = areshape(seqa(25, 1, 24), 2|3|4)")
ge.executeString("c = complex(a, b)")
c = ge.getArray("c")
print ", ".join(str(n) for n in c.getImagData())
```
 *
__PHP__
```php
// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4)");
$ge->executeString("b = areshape(seqa(25, 1, 24), 2|3|4)");
$ge->executeString("c = complex(a, b)");
$c = $ge->getArray("c");
echo implode(", ", $c->getImagData()) . PHP_EOL;
```
 * will result in the output:
```
25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0, 33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0, 40.0, 41.0, 42.0, 43.0, 44.0, 45.0, 46.0, 47.0, 48.0
```
 *
 * @return Array data as a one-dimensional std::vector
 */
std::vector<double> GEArray::getImagData() const {
    if (!isComplex())
        return std::vector<double>();

    std::vector<double> ret(this->num_elements_);

    memcpy(ret.data(), this->data_.data() + this->num_elements_ + this->dims_, this->num_elements_ * sizeof(double));

    return ret;
}

/**
 * Retrieve the std::vector of array orders. These are ordered from highest to lowest.
 *
 * Example:
 *
__Python__
```py
# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")
print "a orders = " + "x".join(str(n) for n in a.getOrders())
```
 *
__PHP__
```php
// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");
echo "a orders = " . implode("x", $a->getOrders()) . PHP_EOL;
```
 * will result in the output:
```
a orders = 2x3x4
```
 *
 * @return        Vector of array orders
 */
std::vector<int> GEArray::getOrders() const {
    if (this->dims_ < 1)
        return std::vector<int>();

    std::vector<int> ret(this->dims_);

    for (int i = 0; i < this->dims_; ++i)
        ret[i] = (int)this->data_[i];

    return ret;
}

/**
 * Return the number of array dimensions. This should be equal to the length of getOrders()
 *
 * Example:
 *
__Python__
```py
# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
a = ge.getArray("a")
print "a dimensions = " + str(a.getDimensions())
print "a order count = " + str(len(a.getOrders()))
```
 *
__PHP__
```php
// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");
$a = $ge->getArray("a");
echo "a dimensions = " . $a->getDimensions() . PHP_EOL;
echo "a order count = " . count($a->getOrders()) . PHP_EOL
```
 * will result in the output:
```
a dimensions = 3
a order count = 3
```
 *
 * @return        Array dimension count
 */
int GEArray::getDimensions() const {
    return this->dims_;
}

/**
 * Returns the total element count. This is the product
 * of getOrders().
 *
 * @return        Total element count
 */
int GEArray::size() const {
    return this->num_elements_;
}

void GEArray::clear() {
    this->data_.resize(3);
    this->data_[0] = 1;
    this->data_[1] = 1;
    this->data_[2] = 0;

    this->dims_ = 2;
    this->num_elements_ = 1;

    GESymbol::clear();
}

Array_t* GEArray::toInternal() {
    const int dims = getDimensions();
    const int sz = size();

    if (!dims || !sz)
        return nullptr;

    Array_t *newArray = new Array_t;

    if (!newArray)
        return nullptr;

    newArray->dims = dims;
    newArray->nelems = sz;
    newArray->complex = static_cast<int>(isComplex());
    newArray->adata = data_.data();
    newArray->freeable = FALSE;

    return newArray;
}
