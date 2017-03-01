#include "gestring.h"

/**
 * Construct an empty Engine compatible string.
 */
GEString::GEString()
{

}

/**
  * Internal use only.
  */
GEString::GEString(String_t *str) {
    setString(str);
}

/**
 * Construct an Engine compatible string with the value of _data_.
 *
 * Example
 *
 * #### Python ####
 * ~~~{.py}
s = GEString("Hello World!")
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = new GEString("Hello World!");
 * ~~~
 *
 * @param data        User-defined string
 */
GEString::GEString(const string &data) {
    setData(data);
}

/**
 * Return a copy of the data as a string
 *
 * #### Python ####
 * ~~~{.py}
ge.executeString("s = \"Hello World!\"")
s = ge.getString("s")
print "s = " + s.getData() # str(s) would be identical
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$ge->executeString("s = \"Hello World!\"");
$s = $ge->getString("s");
echo "s = " . $s->getData() . PHP_EOL; // Specifying '$s' would be identical
 * ~~~
 * would result in output:
 * ~~~
s = Hello World!
 * ~~~
 */
string GEString::getData() const {
    return string(this->data_);
}

/**
 * Sets the data to the value of _str_
 *
 * #### Python ####
 * ~~~{.py}
s = GEString()
s.setData("Hello World!")
print s
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = new GEString();
$s->setData("Hello World!");
echo $s;
 * ~~~
 * would result in output:
 * ~~~
Hello World!
 * ~~~
 *
 */
void GEString::setData(const string &data) {
    this->data_ = data;
}

bool GEString::setString(String_t *str) {
    if (!str)
        return false;

    this->data_ = string(str->stdata);

    GAUSS_Free(str->stdata);
    GAUSS_Free(str);

    return true;
}

/**
 * Returns the length of data.
 *
 * #### Python ####
 * ~~~{.py}
s = GEString()
s.setData("Hello World!")
print "Length of s is " + str(s.size()) + " characters"
 * ~~~
 *
 * #### PHP ####
 * ~~~{.php}
$s = new GEString();
$s->setData("Hello World!");
echo "Length of s is " . $s->size() . " characters" . PHP_EOL;
 * ~~~
 * would result in output:
 * ~~~
Length of s is 12 characters
 * ~~~
 *
 */
int GEString::size() const {
    return data_.length();
}

/**
  * Please see getData().
  */
string GEString::toString() const {
    return this->data_;
}
