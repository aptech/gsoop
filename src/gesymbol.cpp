#include "gesymbol.h"
using namespace std;

GESymbol::GESymbol(int type) :
    rows_(1),
    cols_(1),
    complex_(false),
    type_(type)
{

}

/**
 * Returns the row count.
 *
 * @return Row count
 */
int GESymbol::getRows() const {
    return this->rows_;
}

void GESymbol::setRows(int rows) {
    this->rows_ = rows;
}

/**
 * Returns the column count.
 *
 * @return        Column count
 */
int GESymbol::getCols() const {
    return this->cols_;
}

void GESymbol::setCols(int cols) {
    this->cols_ = cols;
}

/**
 * Returns whether or not the data is complex or not.
 * @return        True if data is complex, false if not.
 */
bool GESymbol::isComplex() const {
    return this->complex_;
}

void GESymbol::setComplex(bool isComplex) {
    this->complex_ = isComplex;
}

/**
 * Returns the total element count for the data.
 *
 * @return        Total element count
 */
int GESymbol::size() const {
    return this->rows_ * this->cols_;
}

GESymbol::~GESymbol() {
    clear();
}
