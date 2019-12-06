#ifndef GEFUNCWRAPPER_H
#define GEFUNCWRAPPER_H

#include <string>
using namespace std;

/**
 * This is the callback function that GAUSS will call to do normal program output.
 * This same callback is also used for error output as well, since a string is
 * passed to both.
 *
 * You will have to provide your own implementation.
 *
 * __Note:__ With all callbacks, the `thisown` flag must be set to `0` on instantiation.
 * This informs the target language that this object will be owned and properly deleted by the Engine.
 *
__Python__
```py
class Output(IGEProgramOutput):
    def invoke(self, message):
        print message,

out = Output()
out.thisown = 0
ge.setProgramOutputAll(out)
ge.executeString("rndseed 12345")
ge.executeString("rndu(3, 3)")  # Will produce valid output
ge.executeString("y")           # Will produce error: y does not exist
```
 *
__PHP__
```php
class Output extends IGEProgramOutput {
    function invoke($message) {
        echo $message;
    }
}

$out = new Output();
$out->thisown = 0;

$ge->setProgramOutput($out);
$ge->executeString("rndseed 12345;");
$ge->executeString("rndu(3, 3);");
```
 *
 * <!--#### Java
```{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
GEProgramOutput outputFn = new GEProgramOutput() {
    public void invoke(String output) {
        System.out.print(output);
    }
};

ge.setProgramOutput(outputFn);
ge.executeString("rndseed 12345;");
ge.executeString("rndu(3, 3);");
```-->
 * will result in the output:
```
      0.90518333       0.49163121       0.23850529
      0.70280651       0.70745944       0.13417575
      0.85034673       0.35349333       0.91067766
```
 *
 * @see GAUSS#setProgramOutputAll(IGEProgramOutput*)
 * @see GAUSS#setProgramOutput(IGEProgramOutput*)
 * @see GAUSS#setProgramErrorOutput(IGEProgramOutput*)
 */
class IGEProgramOutput {
public:
    virtual void invoke(const string &message) = 0;

    virtual ~IGEProgramOutput() {}
};

/**
 * Flush program output callback. The GAUSS Engine will call this if it needs to force
 * printing of any pending output. This function is useful if your IGEProgramOutput class
 * buffers incoming data.
 *
 * __Note:__ With all callbacks, the `thisown` flag must be set to `0` on instantiation.
 * This informs the target language that this object will be owned and properly deleted by the Engine.
 *
__Python__
```py
class FlushOutput(IGEProgramFlushOutput):
    def invoke(self):
        print "A flush was requested."

flush = FlushOutput()
flush.thisown = 0

ge.setProgramFlushOutput(flush)
ge.executeString("print /flush \"Hello World!\"")
```
 *
__PHP__
```php
class FlushOutput extends IGEProgramFlushOutput {
    function invoke() {
        echo "A flush was requested.";
    }
}

$flush = new FlushOutput();
$flush->thisown = 0;

$ge->setProgramFlushOutput($flush);
$ge->executeString("print /flush \"Hello World!\";");
```
 *
 * <!--#### Java
```{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
IGEProgramFlushOutput flushFn = new IGEProgramFlushOutput() {
    public void invoke() {
        System.out.println("A flush was requested.");
    }
};

ge.setProgramFlushOutput(flushFn);
ge.executeString("print /flush \"Hello World!\";");
```-->
 *
 * will result in the output:
```
Hello World!
A flush was requested.
```
 *
 * @see GAUSS#setProgramFlushOutput(IGEProgramFlushOutput*)
 */
class IGEProgramFlushOutput {
public:
    virtual void invoke() = 0;

    virtual ~IGEProgramFlushOutput() {}
};

/**
 * String Input callback function for the GAUSS Engine. Use this class to generate
 * a hook function that the GAUSS Engine will call whenever user string input is required.
 *
 * __Note:__ With all callbacks, the `thisown` flag must be set to `0` on instantiation.
 * This informs the target language that this object will be owned and properly deleted by the Engine.
 *
 * #### GAUSS commands which activate this callback
 * - `cons`
 *
 ** #### Python
```{.py}
class StringInput(IGEProgramInputString):
    # The callback does not return a string directly, rather through a method call.
    def invoke(self, length):
        self.setValue("Hello World!")

strCallback = StringInput();
strCallback.thisown = 0;

ge.setProgramInputString(strCallback)
ge.executeString("s = cons")

s = ge.getString("s")
print "s = " + s
```
 *
__PHP__
```php
class StringInput extends IGEProgramInputString {
    // The callback does not return a string directly, rather through a method call.
    function invoke($length) {
        // Ask user for input normally
        $this->setValue("Hello World!");
    }
}

$strCallback = new StringInput();
$strCallback->thisown = 0;

$ge->setProgramInputString($strCallback);
$ge->executeString("s = cons;");

$s = $ge->getString("s");
echo "s = " . $s . PHP_EOL;
```
 *
 * <!--#### Java `<NOT IMPLEMENTED>`
```{.java}
// The object that implements a callback function MUST stay in scope and not be garbage-collected
// for the entire time that you want to use that particular callback function. If it is prematurely
// garbage-collected, it could cause the JVM to crash.
GEProgramInputString consFn = new GEProgramInputString() {
    GEProgramInputString() {
        this.thisown = 0;
    }

    @Override
    public void invoke(int length) {
        // Ask user for input normally
        this.setValue("Hello World!");
    }
};

ge.setProgramInputString(consFn);
ge.executeString("s = cons;");
String s = ge.getString("s");
System.out.println("s = " + s);
```-->
 * will result in the output:
```
 * s = Hello World!
```
 *
 * @see GAUSS::setProgramInputString(IGEProgramInputString*)
 */
class IGEProgramInputString {
public:
    /**
      * Invocation method for callback. This method is called
      * when the GAUSS Engine needs user supplied input.
      * This callback can also be triggered directly via a call
      * to GAUSS::programInputString()
      *
      * @param length      Max length allowed for return string
      */
    virtual void invoke(int length) = 0;

    virtual ~IGEProgramInputString() {}

    /**
      * Used to set the return value for the callback.
      */
    void setValue(string value) { this->data = value; }

private:
    void clear() { this->setValue(""); }
    string value() { return this->data; }
    string data;

    friend class GAUSS;
};

/**
 * Character input callback function for the GAUSS Engine. This is called
 * when GAUSS requires character input.
 *
 * __Note:__ With all callbacks, the `thisown` flag must be set to `0` on instantiation.
 * This informs the target language that this object will be owned and properly deleted by the Engine.
 *
 * #### GAUSS commands which activate this callback
 * - `keyw`
 * - `key`
 *
__Python__
```py
class CharInput(IGEProgramInputChar):
    def invoke(self):
        # Return buffered input
        return ord("c") # Return the integer value of 'c'

charCallback = CharInput()
charCallback.thisown = 0

ge.setProgramInputCharBlocking(charCallback)
ge.setProgramInputBlocking(charCallback)

ge.executeString("k = key")

k = ge.getScalar("k")
print "k = " + str(k)
```
 *
__PHP__
```php
class CharInput extends IGEProgramInputChar {
    function invoke() {
        // Return character input
        return ord("c"); // Return the integer value of 'c'
    }
}

$charCallback = new CharInput();
$charCallback->thisown = 0;

$ge->setProgramInputChar($charCallback);
$ge->executeString("k = key;");

$k = $ge->getScalar("k");
echo "k = " . $k . PHP_EOL;
```
 * will result in the output:
```
k = 99
```
 *
 * @see GAUSS#setProgramInputChar(IGEProgramInputChar*)
 * @see GAUSS#setProgramInputCharBlocking(IGEProgramInputChar*)
 */
class IGEProgramInputChar {
public:
    virtual int invoke() = 0;

    virtual ~IGEProgramInputChar() {}
};

/**
 * Input check callback function for the GAUSS Engine. This will be called
 * if GAUSS wants to know if there is any input pending.
 *
 * __Note:__ With all callbacks, the `thisown` flag must be set to `0` on instantiation.
 * This informs the target language that this object will be owned and properly deleted by the Engine.
 *
 * #### GAUSS commands which activate this callback
 * - `keyav`
 *
__Python__
```py
class InputCheck(IGEProgramInputCheck):
    def invoke(self):
        # We pretend we have character input available.
        print "Input check requested"
        return 1

inputCheckCallback = InputCheck()
inputCheckCallback.thisown = 0

ge.setProgramInputCheck(inputCheckCallback)
ge.executeString("av = keyav")
ge.executeString("if (av); print \"Key available\"; endif")
```
 *
__PHP__
```php
class InputCheck extends IGEProgramInputCheck {
    function invoke() {
        // We pretend we have character input available.
        echo "Input check requested" . PHP_EOL;
        return 1;
    }
}

$inputCheckCallback = new InputCheck();
$inputCheckCallback->thisown = 0;

$ge->setProgramInputCheck($inputCheckCallback);
$ge->executeString("av = keyav;");
$ge->executeString("if (av); print \"Key available\"; endif;");
```
 * results in output:
```
Input check requested
Key available
```
 *
 * @see GAUSS#setProgramInputCheck(IGEProgramInputCheck*)
 */
class IGEProgramInputCheck {
public:
    virtual bool invoke() = 0;

    virtual ~IGEProgramInputCheck() {}
};

#endif // GEFUNCWRAPPER_H
