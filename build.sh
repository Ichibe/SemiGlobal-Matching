CPPFILE="main.cc gaussian.cc"

CLASSPATH="-classpath .:./Engine/SCommon.jar"

if [ ! -d bin ]; then
    mkdir bin
fi

echo "Conpiling java..."
javac $CLASSPATH SemiGlobalMatchingStorlet.java
mv SemiGlobalMatchingStorlet.class bin/
cd bin
rm -f semiglobalmatchingstorlet-1.0.jar
jar cvf semiglobalmatchingstorlet-1.0.jar SemiGlobalMatchingStorlet.class
rm -f SemiGlobalMatchingStorlet.class
cd ..

echo "Generating header file..."
javah -jni $CLASSPATH:./bin SemiGlobalMatchingStorlet

echo "Compiling cpp"
g++ -shared -fPIC -o libsemiglobalmatching.so \
        -I /usr/lib/jvm/java-8-oracle/include/ -I /usr/lib/jvm/java-8-oracle/include/linux/ \
        -l opencv_nonfree -l opencv_core -l opencv_highgui -l opencv_features2d -l opencv_contrib \
        $CPPFILE
mv libsemiglobalmatching.so bin/
