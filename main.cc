#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <ctime>

#include <string.h>

#include <fstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "SemiGlobalMatchingStorlet.h"

#include "gaussian.h"

#define BLUR_RADIUS 3
#define PATHS_PER_SCAN 4
#define MAX_SHORT std::numeric_limits<unsigned short>::max()
#define SMALL_PENALTY 3
#define LARGE_PENALTY 20
#define DEBUG false

struct path {
    short rowDiff;
    short colDiff;
    short index;
};

void printArray(unsigned short ***array, int rows, int cols, int depth) {
    for (int d = 0; d < depth; ++d) {
        std::cout << "disparity: " << d << std::endl;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                std::cout << "\t" << array[row][col][d];
            }
            std::cout << std::endl;
        }
    }
}

unsigned short calculatePixelCostOneWayBT(int row, int leftCol, int rightCol, const cv::Mat &leftImage, const cv::Mat &rightImage) {

    char leftValue, rightValue, beforeRightValue, afterRightValue, rightValueMinus, rightValuePlus, rightValueMin, rightValueMax;

    if(leftCol<0)
    	leftValue = 0;
    else
	    leftValue = leftImage.at<uchar>(row, leftCol);

	if(rightCol<0)
		rightValue = 0;
	else
	    rightValue = rightImage.at<uchar>(row, rightCol);

    if (rightCol > 0) {
        beforeRightValue = rightImage.at<uchar>(row, rightCol - 1);
    } else {
        beforeRightValue = rightValue;
    }

	//std::cout << rightCol <<" " <<leftCol<< std::endl;   	
    if (rightCol + 1 < rightImage.cols && rightCol>0) {
        afterRightValue = rightImage.at<uchar>(row, rightCol + 1);
    } else {
        afterRightValue = rightValue;
    }

    rightValueMinus = round((rightValue + beforeRightValue) / 2.f);
    rightValuePlus = round((rightValue + afterRightValue) / 2.f);

    rightValueMin = std::min(rightValue, std::min(rightValueMinus, rightValuePlus));
    rightValueMax = std::max(rightValue, std::max(rightValueMinus, rightValuePlus));

    return std::max(0, std::max((leftValue - rightValueMax), (rightValueMin - leftValue)));
}

inline unsigned short calculatePixelCostBT(int row, int leftCol, int rightCol, const cv::Mat &leftImage, const cv::Mat &rightImage) {
    return std::min(calculatePixelCostOneWayBT(row, leftCol, rightCol, leftImage, rightImage),
        calculatePixelCostOneWayBT(row, rightCol, leftCol, rightImage, leftImage));
}

void calculatePixelCost(cv::Mat &firstImage, cv::Mat &secondImage, int disparityRange, unsigned short ***C) {
    for (int row = 0; row < firstImage.rows; ++row) {
        for (int col = 0; col < firstImage.cols; ++col) {
            for (int d = 0; d < disparityRange; ++d) {
            	//std::cout << col << " " << d << " "<<disparityRange << " " << col - d << std::endl;
                C[row][col][d] = calculatePixelCostBT(row, col, col - d, firstImage, secondImage);
            }
        }
    }
}

// pathCount can be 1, 2, 4, or 8
void initializeFirstScanPaths(std::vector<path> &paths, unsigned short pathCount) {
    for (unsigned short i = 0; i < pathCount; ++i) {
        paths.push_back(path());
    }

    if(paths.size() >= 1) {
        paths[0].rowDiff = 0;
        paths[0].colDiff = -1;
        paths[0].index = 1;
    }

    if(paths.size() >= 2) {
        paths[1].rowDiff = -1;
        paths[1].colDiff = 0;
        paths[1].index = 2;
    }

    if(paths.size() >= 4) {
        paths[2].rowDiff = -1;
        paths[2].colDiff = 1;
        paths[2].index = 4;

        paths[3].rowDiff = -1;
        paths[3].colDiff = -1;
        paths[3].index = 7;
    }

    if(paths.size() >= 8) {
        paths[4].rowDiff = -2;
        paths[4].colDiff = 1;
        paths[4].index = 8;

        paths[5].rowDiff = -2;
        paths[5].colDiff = -1;
        paths[5].index = 9;

        paths[6].rowDiff = -1;
        paths[6].colDiff = -2;
        paths[6].index = 13;

        paths[7].rowDiff = -1;
        paths[7].colDiff = 2;
        paths[7].index = 15;
    }
}

// pathCount can be 1, 2, 4, or 8
void initializeSecondScanPaths(std::vector<path> &paths, unsigned short pathCount) {
    for (unsigned short i = 0; i < pathCount; ++i) {
        paths.push_back(path());
    }

    if(paths.size() >= 1) {
        paths[0].rowDiff = 0;
        paths[0].colDiff = 1;
        paths[0].index = 0;
    }

    if(paths.size() >= 2) {
        paths[1].rowDiff = 1;
        paths[1].colDiff = 0;
        paths[1].index = 3;
    }

    if(paths.size() >= 4) {
        paths[2].rowDiff = 1;
        paths[2].colDiff = 1;
        paths[2].index = 5;

        paths[3].rowDiff = 1;
        paths[3].colDiff = -1;
        paths[3].index = 6;
    }

    if(paths.size() >= 8) {
        paths[4].rowDiff = 2;
        paths[4].colDiff = 1;
        paths[4].index = 10;

        paths[5].rowDiff = 2;
        paths[5].colDiff = -1;
        paths[5].index = 11;

        paths[6].rowDiff = 1;
        paths[6].colDiff = -2;
        paths[6].index = 12;

        paths[7].rowDiff = 1;
        paths[7].colDiff = 2;
        paths[7].index = 14;
    }
}

unsigned short aggregateCost(int row, int col, int d, path &p, int rows, int cols, int disparityRange, unsigned short ***C, unsigned short ***A) {
    unsigned short aggregatedCost = 0;
    aggregatedCost += C[row][col][d];

    if(DEBUG) {
        printf("{P%d}[%d][%d](d%d)\n", p.index, row, col, d);
    }

    if (row + p.rowDiff < 0 || row + p.rowDiff >= rows || col + p.colDiff < 0 || col + p.colDiff >= cols) {
        // border
        A[row][col][d] += aggregatedCost;
        if(DEBUG) {
            printf("{P%d}[%d][%d](d%d)-> %d <BORDER>\n", p.index, row, col, d, A[row][col][d]);
        }
        return A[row][col][d];
    }

    unsigned short minPrev, minPrevOther, prev, prevPlus, prevMinus;
    prev = minPrev = minPrevOther = prevPlus = prevMinus = MAX_SHORT;

    for (int disp = 0; disp < disparityRange; ++disp) {
        unsigned short tmp = A[row + p.rowDiff][col + p.colDiff][disp];
        if(minPrev > tmp) {
            minPrev = tmp;
        }

        if(disp == d) {
            prev = tmp;
        } else if(disp == d + 1) {
            prevPlus = tmp;
        } else if (disp == d - 1) {
            prevMinus = tmp;
        } else {
            if(minPrevOther > tmp + LARGE_PENALTY) {
                minPrevOther = tmp + LARGE_PENALTY;
            }
        }
    }

    aggregatedCost += std::min(std::min((int)prevPlus + SMALL_PENALTY, (int)prevMinus + SMALL_PENALTY), std::min((int)prev, (int)minPrevOther + LARGE_PENALTY));
    aggregatedCost -= minPrev;

    A[row][col][d] += aggregatedCost;

    if(DEBUG) {
        printf("{P%d}[%d][%d](d%d)-> %d<CALCULATED>\n", p.index, row, col, d, A[row][col][d]);
    }

    return A[row][col][d];
}

float printProgress(unsigned int current, unsigned int max, int lastProgressPrinted) {
    int progress = floor(100 * current / (float) max);
    if(progress >= lastProgressPrinted + 5) {
        lastProgressPrinted = lastProgressPrinted + 5;
        std::cout << lastProgressPrinted << "%" << std::endl;
    }
    return lastProgressPrinted;
}

void aggregateCosts(int rows, int cols, int disparityRange, unsigned short ***C, unsigned short ****A, unsigned short ***S) {
    std::vector<path> firstScanPaths;
    std::vector<path> secondScanPaths;

    initializeFirstScanPaths(firstScanPaths, PATHS_PER_SCAN);
    initializeSecondScanPaths(secondScanPaths, PATHS_PER_SCAN);

    int lastProgressPrinted = 0;
    std::cout << "First scan..." << std::endl;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            for (unsigned int path = 0; path < firstScanPaths.size(); ++path) {
                for (int d = 0; d < disparityRange; ++d) {
                    S[row][col][d] += aggregateCost(row, col, d, firstScanPaths[path], rows, cols, disparityRange, C, A[path]);
                }
            }
        }
        lastProgressPrinted = printProgress(row, rows - 1, lastProgressPrinted);
    }

    lastProgressPrinted = 0;
    std::cout << "Second scan..." << std::endl;
    for (int row = rows - 1; row >= 0; --row) {
        for (int col = cols - 1; col >= 0; --col) {
            for (unsigned int path = 0; path < secondScanPaths.size(); ++path) {
                for (int d = 0; d < disparityRange; ++d) {
                    S[row][col][d] += aggregateCost(row, col, d, secondScanPaths[path], rows, cols, disparityRange, C, A[path]);
                }
            }
        }
        lastProgressPrinted = printProgress(rows - 1 - row, rows - 1, lastProgressPrinted);
    }
}

void computeDisparity(unsigned short ***S, int rows, int cols, int disparityRange, cv::Mat &disparityMap) {
    unsigned int disparity = 0, minCost;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            minCost = MAX_SHORT;
            for (int d = disparityRange - 1; d >= 0; --d) {
                if(minCost > S[row][col][d]) {
                    minCost = S[row][col][d];
                    disparity = d;
                }
            }
            disparityMap.at<uchar>(row, col) = disparity;
        }
    }
}


// ichibe changed 
//void saveDisparityMap(cv::Mat &disparityMap, int disparityRange, char* outputFile) {
void saveDisparityMap(cv::Mat &disparityMap, int disparityRange) {
    double factor = 256.0 / disparityRange;
    for (int row = 0; row < disparityMap.rows; ++row) {
        for (int col = 0; col < disparityMap.cols; ++col) {
            disparityMap.at<uchar>(row, col) *= factor;
        }
    }
    //cv::imwrite(outputFile, disparityMap);
}

/*
 * JNIEXPORT jbyteArray JNICALL Java_SemiGlobalMatchingStorlet_process
    (JNIEnv *env, jobject me, jbyteArray inBytes1, jbyteArray inBytes2)
{
    jbyteArray outBytesj = env->NewByteArray(sizeof(uchar) * 1);
    return outBytesj;
}
*/

JNIEXPORT jbyteArray JNICALL Java_SemiGlobalMatchingStorlet_process
    (JNIEnv *env, jobject me, jbyteArray inBytes1, jbyteArray inBytes2)
{
    jboolean b;
    jbyte* firstBuf = env->GetByteArrayElements(inBytes1, &b);
    jsize fbuf_len = env->GetArrayLength(inBytes1);
    jbyte* secondBuf = env->GetByteArrayElements(inBytes2, &b);
    jsize sbuf_len = env->GetArrayLength(inBytes2);
    
    cv::Mat firstImage;
    cv::Mat secondImage;
    //firstImage = cv::imread(firstFileName, CV_LOAD_IMAGE_GRAYSCALE);
    //secondImage = cv::imread(secondFileName, CV_LOAD_IMAGE_GRAYSCALE);

    //char *outFileName="./result.png";
    unsigned int disparityRange = 24;

    //printf("%d , %d\n", fbuf_len, sbuf_len);
    //printf("char size:%ld \n", sizeof(char));
    firstImage = cv::imdecode(cv::Mat(cv::Size(1, fbuf_len), CV_8UC1, (void*)firstBuf), CV_LOAD_IMAGE_GRAYSCALE);
    secondImage = cv::imdecode(cv::Mat(cv::Size(1, sbuf_len), CV_8UC1, (void*)secondBuf), CV_LOAD_IMAGE_GRAYSCALE);

    if(!firstImage.data || !secondImage.data) {
        std::cerr <<  "Could not open or find one of the images!" << std::endl;
        return NULL;
    }

    unsigned short ***C; // pixel cost array W x H x D
    unsigned short ***S; // aggregated cost array W x H x D
    unsigned short ****A; // single path cost array 2 x W x H x D

    /*
    * TODO
    * variable LARGE_PENALTY
    */

    clock_t begin = clock();

    //std::cout << "Allocating space..." << std::endl;

    // allocate cost arrays
    C = new unsigned short**[firstImage.rows];
    S = new unsigned short**[firstImage.rows];
    for (int row = 0; row < firstImage.rows; ++row) {
        C[row] = new unsigned short*[firstImage.cols];
        S[row] = new unsigned short*[firstImage.cols]();
        for (int col = 0; col < firstImage.cols; ++col) {
            C[row][col] = new unsigned short[disparityRange];
            S[row][col] = new unsigned short[disparityRange](); // initialize to 0
        }
    }

    A = new unsigned short ***[PATHS_PER_SCAN];
    for(int path = 0; path < PATHS_PER_SCAN; ++path) {
        A[path] = new unsigned short **[firstImage.rows];
        for (int row = 0; row < firstImage.rows; ++row) {
            A[path][row] = new unsigned short*[firstImage.cols];
            for (int col = 0; col < firstImage.cols; ++col) {
                A[path][row][col] = new unsigned short[disparityRange];
                for (unsigned int d = 0; d < disparityRange; ++d) {
                    A[path][row][col][d] = 0;
                }
            }
        }
    }

    //std::cout << "Smoothing images..." << std::endl;
    grayscaleGaussianBlur(firstImage, firstImage, BLUR_RADIUS);
    grayscaleGaussianBlur(secondImage, secondImage, BLUR_RADIUS);

    //std::cout << "Calculating pixel cost for the image..." << std::endl;
    calculatePixelCost(firstImage, secondImage, disparityRange, C);
    if(DEBUG) {
        printArray(C, firstImage.rows, firstImage.cols, disparityRange);
    }
    //std::cout << "Aggregating costs..." << std::endl;
    aggregateCosts(firstImage.rows, firstImage.cols, disparityRange, C, A, S);

    cv::Mat disparityMap = cv::Mat(cv::Size(firstImage.cols, firstImage.rows), CV_8UC1, cv::Scalar::all(0));

    //std::cout << "Computing disparity..." << std::endl;
    computeDisparity(S, firstImage.rows, firstImage.cols, disparityRange, disparityMap);

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    //printf("Done in %.2lf seconds.\n", elapsed_secs);

    saveDisparityMap(disparityMap, disparityRange);

    std::vector<uchar> outbuf;
    std::vector<int> params(2);
    params[0] = CV_IMWRITE_PNG_COMPRESSION;
    params[1] = 9;
    cv::imencode(".png", disparityMap, outbuf, params);
    //printf("length:%d\n", (int)outbuf.size());
    
    //size_t disparityMapLen = disparityMap.cols * disparityMap.rows;
    size_t disparityMapLen = outbuf.size();
    //printf("disparityMapLen:%ld\n", (long int)disparityMapLen);
    jbyteArray outBytesj = env->NewByteArray(sizeof(uchar) * disparityMapLen);
    if (outBytesj == NULL) {
        env->ReleaseByteArrayElements(inBytes1, firstBuf, 0);
        env->ReleaseByteArrayElements(inBytes2, secondBuf, 0);
        printf("outBytesj is NULL");
        return NULL;
    }
    jbyte* outBytes = env->GetByteArrayElements(outBytesj, NULL);
    if (outBytes == NULL) {
        env->ReleaseByteArrayElements(inBytes1, firstBuf, 0);
        env->ReleaseByteArrayElements(inBytes2, secondBuf, 0);
        printf("outBytes is NULL");
        return NULL;
    }
    if(disparityMap.data == NULL ){
        printf("disparityMap.data is NULL");
    }
    //printf("size of float :%d\n", (int)sizeof(uchar));
    //printf("size of float :%d\n", (int)sizeof(double));
    memcpy(outBytes, &outbuf[0], sizeof(uchar)*disparityMapLen);
    //printf("fnish\n");
    
   //delete buffer
   for (int row = 0; row < firstImage.rows; ++row) {
       for (int col = 0; col < firstImage.cols; ++col) {
            delete C[row][col];
            delete S[row][col]; // initialize to 0
        }
        delete[] C[row];
        delete[] S[row];
    }
    delete[] C;
    delete[] S;
 
    for(int path = 0; path < PATHS_PER_SCAN; ++path) {
        for (int row = 0; row < firstImage.rows; ++row) {
            for (int col = 0; col < firstImage.cols; ++col) {
                //for (unsigned int d = 0; d < disparityRange; ++d) {
                //    delete A[path][row][col][d];
                //}
		delete[] A[path][row][col];
            }
	    delete[] A[path][row];
        }
        delete[] A[path];
    }

    delete[] A;

    firstImage.release();
    secondImage.release();
    disparityMap.release();

    // Release all arrays
    env->ReleaseByteArrayElements(inBytes1, firstBuf, 0);
    env->ReleaseByteArrayElements(inBytes2, secondBuf, 0);
    env->ReleaseByteArrayElements(outBytesj, outBytes, 0);

    return outBytesj;
}

