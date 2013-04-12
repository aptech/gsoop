#include "gesymbol.h"
using namespace std;

GESymbol::GESymbol() {
    this->rows_ = 1;
    this->cols_ = 1;
}

/**
 * Returns the row count.
 *
 * @return Row count
 */
int GESymbol::getRows() {
    return this->rows_;
}

void GESymbol::setRows(int rows) {
    this->rows_ = rows;
}

/**
 * Returns the column count.
 *
 * @return	Column count
 */
int GESymbol::getCols() {
    return this->cols_;
}

void GESymbol::setCols(int cols) {
    this->cols_ = cols;
}

/**
 * Returns whether or not the data is complex or not.
 * @return	True if data is complex, false if not.
 */
bool GESymbol::isComplex() {
    return this->complex_;
}

void GESymbol::setComplex(bool isComplex) {
    this->complex_ = isComplex;
}

/**
 * Returns the total element count for the data.
 *
 * @return	Total element count
 */
int GESymbol::size() {
    return this->rows_ * this->cols_;
}

GESymbol::~GESymbol() {
    clear();
}
