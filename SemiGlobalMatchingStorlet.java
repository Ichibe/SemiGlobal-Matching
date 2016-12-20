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

/*
 * Author: kajinamit
 */
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;
import java.io.BufferedInputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import org.openstack.storlet.common.IStorlet;
import org.openstack.storlet.common.StorletException;
import org.openstack.storlet.common.StorletInputStream;
import org.openstack.storlet.common.StorletLogger;
import org.openstack.storlet.common.StorletOutputStream;
import org.openstack.storlet.common.StorletObjectOutputStream;


public class SemiGlobalMatchingStorlet implements IStorlet {

    static{
        System.loadLibrary("semiglobalmatching");
    }

    native byte[] process(byte[] inBytes1, byte[] inBytes2);

	/***
	 * Storlet invoke method.
	 * 
	 * @throws StorletException
	 */
	@Override
	public void invoke(ArrayList<StorletInputStream> inputStreams,
			ArrayList<StorletOutputStream> outputStreams,
			Map<String, String> params, StorletLogger logger)
			throws StorletException {
        logger.emitLog("1");
	logger.emitLog("size:" + inputStreams.size());
        InputStream inputStream = inputStreams.get(0).getStream();
        logger.emitLog("2");
        InputStream inputStream1 = inputStreams.get(1).getStream();
        logger.emitLog("3");
        final HashMap<String, String> metadata = inputStreams.get(0)
                .getMetadata();
        logger.emitLog("4");
        final StorletObjectOutputStream storletObjectOutputStream = (StorletObjectOutputStream) outputStreams
                .get(0);

        logger.emitLog("5");
        OutputStream outputStream = storletObjectOutputStream.getStream();

        logger.emitLog("6");
        int readLength = 512;
        logger.emitLog("7");
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream(readLength);
        logger.emitLog("8");
        ByteArrayOutputStream byteStream1 = new ByteArrayOutputStream(readLength);
        logger.emitLog("9");
        byte[] bytes = new byte[readLength];
        logger.emitLog("10");
        BufferedInputStream bis = new BufferedInputStream(inputStream, readLength);
        logger.emitLog("11");
        BufferedInputStream bis1 = new BufferedInputStream(inputStream1, readLength);

        // Red data from stream
        logger.emitLog("Reading data from inputStream");
        try{
            int len = 0;
            while ((len = bis.read(bytes, 0, readLength)) > 0) {
                byteStream.write(bytes, 0, len);
            }
        } catch (IOException e) {
            logger.emitLog("Failed to read data from inputStream");
            logger.Flush();
            throw new StorletException(e.getMessage());
        }
        byte[] inBytes = byteStream.toByteArray();

        try{
            int len = 0;
            while ((len = bis1.read(bytes, 0, readLength)) > 0) {
                byteStream1.write(bytes, 0, len);
            }
        } catch (IOException e) {
            logger.emitLog("Failed to read data from inputStream");
            logger.Flush();
            throw new StorletException(e.getMessage());
        }

        byte[] inBytes1 = byteStream1.toByteArray();
        long start = System.currentTimeMillis();
        logger.emitLog("Start processing data1");
        byte[] outBytes = null;
        try{
            outBytes = process(inBytes, inBytes1);
        } catch (Exception e) {
            logger.emitLog("Failed to process  inputStream");
            logger.emitLog(e.getMessage());
            logger.Flush();
            throw new StorletException(e.getMessage());
        }
        //byte[] outBytes = inBytes;
        long end = System.currentTimeMillis();
        logger.emitLog(String.format("TIME,%d,%d,%d,%d", start, end-start, inBytes1.length, outBytes.length));

        storletObjectOutputStream.setMetadata(metadata);
        logger.emitLog("Sending back object data");
        try{
            // Write raw data
            outputStream.write(outBytes);
            // Close streams
            inputStream.close();
            outputStream.close();
        } catch (IOException e) {
		    logger.emitLog(e.getMessage());
			throw new StorletException(e.getMessage());
		}
	}
}
