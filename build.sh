CPPFILE="main.cc gaussian.cc"

CLASSPATH="-classpath .:./Engine/SCommon.jar"

if [ ! -d bin ]; then
    mkdir bin
fi

# echo "Compiling java..."
#javac $CLASSPATH SemiGlobalMatchingStorlet.java
#mv SemiGlobalMatchingStorlet.class bin/
#cd bin
#rm -f semiglobalmatchingstorlet-1.0.jar
#jar cvf semiglobalmatchingstorlet-1.0.jar SemiGlobalMatchingStorlet.class
#rm -f SemiGlobalMatchingStorlet.class
#cd ..

echo "Generating header file..."
javah -jni $CLASSPATH:./bin SemiGlobalMatchingStorlet

echo "Compiling cpp"
# g++ -shared -fPIC -O3 -o libsemiglobalmatching.so \
g++ -shared -fPIC -o libsemiglobalmatching.so \
        -Wl,-rpath,/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu \
        -I ${JAVA_HOME}/include/ -I ${JAVA_HOME}/include/linux/ \
        -l opencv_nonfree -l opencv_core -l opencv_highgui -l opencv_features2d -l opencv_contrib \
        $CPPFILE
mv libsemiglobalmatching.so bin/
