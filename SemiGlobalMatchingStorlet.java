/*----------------------------------------------------------------------------
 * Copyright IBM Corp. 2015, 2015 All Rights Reserved
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * Limitations under the License.
 * ---------------------------------------------------------------------------
 */

import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;
import java.io.BufferedInputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;


public class SemiGlobalMatchingStorlet {

    static{
        System.loadLibrary("semiglobalmatching");
    }

    public native byte[] process(byte[] inBytes1, byte[] inBytes2);

    /***
     ** Storlet invoke method.
     *
     * @throws StorletException
     */
    public static void main(String[] args){
        FileInputStream left, right;
        FileOutputStream outputStream;
        try{
            left = new FileInputStream("./test/bull/left.png");
            right = new FileInputStream("./test/bull/right.png");

            outputStream = new FileOutputStream("./test_output.png");
        } catch (IOException e) {
            System.out.println("fail to open");
        return;
    }
        int readLength = 64 * 1024;
        // int readLength = 512;
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream(readLength);
        ByteArrayOutputStream byteStream1 = new ByteArrayOutputStream(readLength);
        byte[] bytes = new byte[readLength];
        BufferedInputStream bis = new BufferedInputStream(left, readLength);
        BufferedInputStream bis1 = new BufferedInputStream(right, readLength);

        // Red data from stream
        try{
            int len = 0;
            while ((len = bis.read(bytes, 0, readLength)) > 0) {
                byteStream.write(bytes, 0, len);
            }
        } catch (IOException e) {
            System.out.println("fail to read");
        return;
        }
        byte[] inBytes = byteStream.toByteArray();

        try{
            int len = 0;
            while ((len = bis1.read(bytes, 0, readLength)) > 0) {
                byteStream1.write(bytes, 0, len);
            }
        } catch (IOException e) {
            System.out.println("fail to read");
            return;
        }

        byte[] inBytes1 = byteStream1.toByteArray();
        long start = System.currentTimeMillis();
        byte[] outBytes = null;
        try{
            SemiGlobalMatchingStorlet storlet = new SemiGlobalMatchingStorlet();

            outBytes = storlet.process(inBytes, inBytes1);
        } catch (Exception e) {
            System.out.println("fail to process");
            return;
        }
        //byte[] outBytes = inBytes;
        long end = System.currentTimeMillis();

        try{
            // Write raw data
            outputStream.write(outBytes);
            // outputStream.write("".getBytes());
            // Close streams
            left.close();
            right.close();
            outputStream.close();
        } catch (IOException e) {
            System.out.println("fail to close");
            return;
        }
    }
}
