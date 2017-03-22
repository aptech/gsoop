#include "gematrix.h"
#include <cstring>
#include <sstream>
using namespace std;

/**
 * Construct a `1x1` matrix with value of `0`.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
x = GEMatrix()
ge.setSymbol(x, "x")
ge.executeString("print x")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$x = new GEMatrix();
$ge->setSymbol($x, "x");
$ge->executeString("print x;");
 * ~~~
 * results in output:
 * ~~~
       0.0000000
 * ~~~
 */
GEMatrix::GEMatrix() : GESymbol()
{
    GEMatrix(0.0);
}

GEMatrix::~GEMatrix() {
    
}

/**
 * Initialize matrix to scalar with specified value.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
x = GEMatrix(1.0)
ge.setSymbol(x, "x")
ge.executeString("print x")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$x = new GEMatrix(1.0);
$ge->setSymbol($x, "x");
$ge->executeString("print x;");
 * ~~~
 * results in output:
 * ~~~
       1.0000000
 * ~~~
 *
 * @param d Scalar value
 */
GEMatrix::GEMatrix(double n) : GESymbol() {
	this->data_->push_back(n);
    this->setRows(1);
    this->setCols(1);
    this->setComplex(false);
}

/**
* Internal use only for use with GetMatrixAndClear.
*/
GEMatrix::GEMatrix(Matrix_t* mat) : GESymbol() {
	if (mat == NULL)
		return;

	this->setRows(mat->rows);
	this->setCols(mat->cols);
	this->setComplex(static_cast<bool>(mat->complex));

	int elements = size() * (isComplex() ? 2 : 1);

	this->data_;
	this->data_->resize(elements);
	memcpy(this->data_->data(), mat->mdata, elements * sizeof(double));

	GAUSS_Free(mat->mdata); // We have ownership of original from symbol table
	GAUSS_Free(mat);
}


/**
  * Internal use only.
  */
GEMatrix::GEMatrix(GAUSS_MatrixInfo_t* mat) : GESymbol() {
    if (mat == NULL)
        return;

	this->setRows(mat->rows);
    this->setCols(mat->cols);
    this->setComplex(static_cast<bool>(mat->complex));

	int elements = size() * (isComplex() ? 2 : 1);

	this->data_->resize(elements);
	memcpy(this->data_->data(), mat->maddr, elements * sizeof(double));

	delete mat;
}

/**
 * Construct a matrix object from a vector of double-precision numbers with `data.size()` columns and 1 row.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
x = GEMatrix([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0])
ge.setSymbol(x, "x")
ge.executeString("print x")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$x = new GEMatrix(array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));
$ge->setSymbol($x, "x");
$ge->executeString("print x;");
 * ~~~
 * results in output:
 * ~~~
       1.0000000        2.0000000        3.0000000        4.0000000        5.0000000        6.0000000        7.0000000        8.0000000
 * ~~~
 *
 * @param   data    Data vector
 *
 * @see GEMatrix(vector<double>, int, int, bool)
 * @see GEMatrix(vector<double>, vector<double>, int, int)
 */
GEMatrix::GEMatrix(const vector<double> &data) : GESymbol() {
    Init(data, 1, data.size(), false);
}

/**
 * Construct a real matrix with _rows_ rows, and _cols_ columns.
 *
 * If _complex_ is set to __true__, `data.length()` must be
 * _rows_ * _cols_ * 2
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
# Create a matrix
x = GEMatrix([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0], 4, 2)
ge.setSymbol(x, "x")
ge.executeString("print x")

# Create a matrix with imaginary data
xc = GEMatrix([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0], 2, 2, True)
ge.setSymbol(xc, "xc")
ge.executeString("print xc")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
// Create a matrix
$x = new GEMatrix(array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0), 4, 2);
$ge->setSymbol($x, "x");
$ge->executeString("print x;");

// Create a matrix with imaginary data
$xc = new GEMatrix(array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0), 2, 2, true);
$ge->setSymbol($xc, "xc");
$ge->executeString("print xc;");
 * ~~~
 * results in output:
 * ~~~
       1.0000000        2.0000000
       3.0000000        4.0000000
       5.0000000        6.0000000
       7.0000000        8.0000000

       1.0000000 +        5.0000000i        2.0000000 +        6.0000000i
       3.0000000 +        7.0000000i        4.0000000 +        8.0000000i
 * ~~~
 *
 * @param data      Vector of double precision values
 * @param rows      Number of rows
 * @param cols      Number of columns
 * @param complex   True if including imaginary data, false otherwise
 *
 * @see GEMatrix(vector<double>, vector<double>, int, int)
 */
GEMatrix::GEMatrix(const vector<double> &data, int rows, int cols, bool complex) : GESymbol() {
    const double *imag_data = complex ? &data[0] + rows * cols : NULL;
    Init(&data[0], imag_data, rows, cols, complex);
}

/**
 * Construct a complex matrix. Uses _real_data_ for real data and
 * _imag_data_ for imaginary data, with _rows_ rows, and _cols_ columns.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
# Create a matrix with imaginary data
xc = GEMatrix([1.0, 2.0, 3.0, 4.0], [5.0, 6.0, 7.0, 8.0], 2, 2)
ge.setSymbol(xc, "xc")
ge.executeString("print xc")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
// Create a matrix with imaginary data
$xc = new GEMatrix(array(1.0, 2.0, 3.0, 4.0), array(5.0, 6.0, 7.0, 8.0), 2, 2);
$ge->setSymbol($xc, "xc");
$ge->executeString("print xc;");
 * ~~~
 * results in output:
 * ~~~
       1.0000000 +        5.0000000i        2.0000000 +        6.0000000i
       3.0000000 +        7.0000000i        4.0000000 +        8.0000000i
 * ~~~
 *
 * @param real_data      Vector of real double precision values
 * @param imag_data      Vector of imaginary double precision values
 * @param rows      Number of rows
 * @param cols      Number of columns
 * @param complex   True if including imaginary data, false otherwise
 *
 * @see GEMatrix(vector<double>, int, int, bool)
 */
GEMatrix::GEMatrix(const vector<double> &real_data, const vector<double> &imag_data, int rows, int cols) : GESymbol() {
    Init(real_data, imag_data, rows, cols, true);
}

GEMatrix::GEMatrix(const double *data, int rows, int cols, bool complex) : GESymbol() {
    const double *imag_data = complex ? data + rows * cols : NULL;
    Init(data, imag_data, rows, cols, complex);
}

GEMatrix::GEMatrix(const double *data, const double *imag_data, int rows, int cols) : GESymbol() {
    Init(data, imag_data, rows, cols, true);
}

void GEMatrix::Init(const vector<double> &data, int rows, int cols, bool complex) {
	const double *imag_data = complex ? &data[0] + rows * cols : NULL;
	Init(&data[0], imag_data, rows, cols, complex);
}

void GEMatrix::Init(const vector<double> &real_data, const vector<double> &imag_data, int rows, int cols, bool complex) {
    // Validate input
    if (real_data.empty())
        return;
    else if (complex && imag_data.empty())
        return;

    Init(&real_data[0], &imag_data[0], rows, cols, complex);
}

void GEMatrix::Init(const double *p_real_data, const double *p_imag_data, int rows, int cols, bool complex) {
    if (!p_real_data)
        return;
    else if (complex && !p_imag_data)
        return;

    this->setRows(rows);
    this->setCols(cols);
    this->setComplex(complex);

    int elements = rows * cols;

	this->data_->resize(elements * (complex ? 2 : 1));

    memcpy(this->data_->data(), p_real_data, elements * sizeof(double));

    if (complex)
        memcpy(this->data_->data() + elements, p_imag_data, elements * sizeof(double));
}

void GEMatrix::clear() {
	this->data_->resize(1);
	this->data_[0] = 0.0;
	GESymbol::clear();
}

/**
 * Set the first value in matrix. This method can be used to set both
 * real and imaginary values, determined by the _imag_ flag.
 *
 * @param val        Value to set
 * @param imag        True if setting imaginary data, false if setting real data.
 *
 * @see setElement(double, int, int, bool)
 */
bool GEMatrix::setElement(double value, bool imag) {
    if (this->data_->empty() || (!isComplex() && imag))
        return false;

	int index = imag ? this->size() : 0;

	this->data_[index] = value;

    return true;
}

/**
 * Returns the first value from a matrix. This will return
 * real or imaginary data depending on the _imag_ flag passed in. If you only
 * are interested in seeing real data you can use the convenience method
 * getElement(int, int) that is also available.
 *
 * @param imag        True for imaginary data, false for real
 * @return        Double precision number at row/column coordinates.
 *
 * @see getElement(int, int, bool)
 */
double GEMatrix::getElement(bool imag) const {
    if (this->data_->empty())
        return false;

    return this->data_[imag ? this->size() : 0];
}

/**
 * Returns a value from the matrix at the specified row/column index. This will return
 * real or imaginary data depending on the _imag_ flag passed in. If you only
 * are interested in seeing real data you can use the convenience method
 * getElement(int, int) that is also available.
 *
 * @param row        Row index
 * @param col        Column index
 * @param imag        True for imaginary data, false for real
 * @return        Double precision number at row/column coordinates.
 *
 * @see getElement(bool)
 */
double GEMatrix::getElement(int row, int col, bool imag) const {
    if (this->data_->empty() || (!isComplex() && imag))
        return 0;
	else if (row >= this->getRows() || col >= this->getCols())
        return 0;
	
	int index = row * getCols() + col + (imag ? size() : 0);

	return this->data_->at(index);
}

/**
 * Set a value in the matrix at the specified row/column index. This
 * method can be used to set both real and imaginary values, determined
 * by the _imag_ flag.
 *
 * @param row        Row index
 * @param col        Column index
 * @param val        Value to set
 * @param imag        True if setting imaginary data, false if setting real data.
 *
 * @see setElement(double, bool)
 */
bool GEMatrix::setElement(double value, int row, int col, bool imag) {
	if (this->data_->empty() || (!isComplex() && imag))
		return false;
	else if (row >= this->getRows() || col >= this->getCols())
		return false;

	int index = row * getCols() + col + (imag ? size() : 0);

	this->data_[index] = value;

	return true;
}

/**
 * Retrieve a copy of underlying numeric vector. Imaginary data can also
 * be queried by supplying the _imag_ argument, thus appending it to the
 * end of the real data. Omitting or specifying the _imag_ argument as `false`
 * will return only real data. If you wish to only view the imaginary data,
 * there is a getImagData() method for convenience.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
# Create a 4x2 matrix of integers 1 - 8.
ge.executeString("x = reshape(seqa(1, 1, 8), 4, 2)")
ge.executeString("print x")
x = ge.getMatrix("x")
print ", ".join(str(n) for n in x.getData())
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
// Create a 4x2 matrix of integers 1 - 8.
$ge->executeString("x = reshape(seqa(1, 1, 8), 4, 2);");
$ge->executeString("print x;");
$x = $ge->getMatrix("x");
echo implode(", ", $x->getData());
 * ~~~
 * results in output:
 * ~~~
       1.0000000        2.0000000
       3.0000000        4.0000000
       5.0000000        6.0000000
       7.0000000        8.0000000
1, 2, 3, 4, 5, 6, 7, 8
 * ~~~
 *
 * @param imag  Whether to return imaginary data as well.
 *
 * @return        Double precision vector of data
 *
 * @see getImagData()
 */
vector<double> GEMatrix::getData(bool imag) const {
	if (imag && !isComplex()) {
		return vector<double>();
	} else if (imag == isComplex()) {
		return this->data_;
	}

	int elements = this->size();
	vector<double> ret(elements);

	memcpy(ret.data(), this->data_->data(), elements * sizeof(double));

	return ret;
}

/**
 * Retrieve a copy of underlying imaginary data storage.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
# Create two 2x2 matrices and combine to form a complex matrix.
ge.executeString("x_real = reshape(seqa(1, 1, 4), 2, 2)")
ge.executeString("x_imag = reshape(seqa(5, 1, 4), 2, 2)")
ge.executeString("x = complex(x_real, x_imag)")
x = ge.getMatrix("x")
print ", ".join(str(n) for n in x.getImagData())
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
// Create two 2x2 matrices and combine to form a complex matrix.
$ge->executeString("x_real = reshape(seqa(1, 1, 4), 2, 2);");
$ge->executeString("x_imag = reshape(seqa(5, 1, 4), 2, 2);");
$ge->executeString("x = complex(x_real, x_imag);");
$x = $ge->getMatrix("x");
echo implode(", ", $x->getImagData());
 * ~~~
 * results in output:
 * ~~~
5, 6, 7, 8
 * ~~~
 *
 * @return        Double precision vector of data
 *
 * @see getData()
 */
vector<double> GEMatrix::getImagData() const {
	vector<double> ret;

	if (!isComplex())
		return ret;

	int elements = this->size();

	ret.resize(elements);

	memcpy(ret.data(), this->data_->data() + elements, elements * sizeof(double));

	return ret;
}

string GEMatrix::toString() const {
    stringstream s;

    int rows = getRows();
    int cols = getCols();
    bool complex = isComplex();

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            s << getElement(i, j);

            if (complex)
                s << " + " << getElement(i, j, true);

            if (j < cols - 1)
                s << "\t";
        }

        if (i < rows - 1)
            s << endl;
    }

    return s.str();
}
