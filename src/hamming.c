#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Definitions
#define block uint32_t // 32 bits
#define bit bool       // 8 bits (only last is used)
#define BITS_PER_BLOCK 30
#define MESSAGE_BITS_PER_BLOCK 24
#define PARITY_BITS_NUM (BITS_PER_BLOCK - MESSAGE_BITS_PER_BLOCK)

typedef struct Block
{
    block *blockArray;
    uint64_t blockNum;
} Block_t;

// Function prototypes
void decode(block *input, uint64_t blockNum, Block_t *ptr); // Function used to decode Hamming code
void encode(uint32_t *input, int len, Block_t *ptr);        // Function used to encode plaintext
void printBlock(block i);                                   // Function used to pretty print a block
int multipleXor(uint8_t *indicies, uint8_t len);            // Function used to XOR all the elements of a list together (used to locate error and determine values of parity bits)

int main(int argc, char const *argv[])
{

    srand((uint32_t)time(NULL));

    uint32_t data[] = {0xFFFFFF, 0x000007};
    Block_t myBlock;
    encode(data, 2 * MESSAGE_BITS_PER_BLOCK / 8, &myBlock);

    printf("\nDecode --- No Error Bits:\n");
    // decode(myBlock.blockArray, myBlock.blockNum, NULL);

    printf("\nDecode --- One Error Bit:\n");
    uint32_t temp1 = myBlock.blockArray[0];
    uint32_t temp2 = myBlock.blockArray[1];
    for (uint32_t errorBitIndex = 0; errorBitIndex < MESSAGE_BITS_PER_BLOCK; errorBitIndex++)
    {
        /* code */
        for (uint32_t i = 0; i < myBlock.blockNum; i++)
        {
            // uint32_t errorBitIndex = rand() % 24;
            // uint32_t errorBitIndex = 0;
            myBlock.blockArray[i] ^= (1 << (errorBitIndex));
        }
        decode(myBlock.blockArray, myBlock.blockNum, NULL);
        assert(temp1 == myBlock.blockArray[0]);
        assert(temp2 == myBlock.blockArray[1]);
    }
    for (uint32_t errorBitIndex = MESSAGE_BITS_PER_BLOCK + 1; errorBitIndex < BITS_PER_BLOCK; errorBitIndex++)
    {
        /* code */
        for (uint32_t i = 0; i < myBlock.blockNum; i++)
        {
            // uint32_t errorBitIndex = rand() % 24;
            // uint32_t errorBitIndex = 0;
            myBlock.blockArray[i] ^= (1 << (errorBitIndex));
        }
        decode(myBlock.blockArray, myBlock.blockNum, NULL);
        assert(temp1 == myBlock.blockArray[0]);
        assert(temp2 == myBlock.blockArray[1]);
    }

    printf("\nDecode --- Two Error Bits:\n");
    for (uint32_t i = 0; i < myBlock.blockNum; i++)
    {
        uint32_t errorBitIndex = rand() % 6 + 24;
        // uint32_t errorBitIndex = 2;
        myBlock.blockArray[i] ^= (1 << (errorBitIndex));
        errorBitIndex = rand() % 24;
        // errorBitIndex = 3;
        myBlock.blockArray[i] ^= (1 << (errorBitIndex));
    }
    decode(myBlock.blockArray, myBlock.blockNum, NULL);

    return 0;
}

/**
 * @brief 编码
 *
 * @param input 输入的 Message，不应超过 30 bit，即最高位和次高位必须为0
 * @param len 输入 Message 数量
 * @param ptr 编码结果保存到的结构体
 */
void encode(uint32_t *input, int len, Block_t *ptr)
{
    // 输入的 Message，不应超过 30 bit
    for (uint32_t inputIndex = 0; inputIndex < len; inputIndex++)
    {
        assert(input[inputIndex] < 0xC0000000);
    }
    
    int blockNum = ceil((float)len * 8 / MESSAGE_BITS_PER_BLOCK);

    // block blockArray[blockNum];
    block *blockArray = (block *)malloc(sizeof(block) * blockNum);
    memset(blockArray, 0, sizeof(block) * blockNum);

    uint64_t currentByteIndex = 0;

    for (int blockIndex = 0; blockIndex < blockNum; blockIndex++)
    {
        uint64_t skip = 2;
        printf("On block %d\n", blockIndex);
        currentByteIndex = (MESSAGE_BITS_PER_BLOCK / 8) * blockIndex;
        uint8_t oneBitIndex[BITS_PER_BLOCK];
        uint8_t oneCount = 0;

        // 确定 block 前 24 bit
        if (currentByteIndex + MESSAGE_BITS_PER_BLOCK / 8 <= len)
        {
            memcpy(&blockArray[blockIndex], &input[blockIndex], MESSAGE_BITS_PER_BLOCK / 8);
        }
        else
        {
            memcpy(&blockArray[blockIndex], &input[blockIndex], len - currentByteIndex);
            memset(&blockArray[blockIndex + len - currentByteIndex], 0, currentByteIndex + MESSAGE_BITS_PER_BLOCK / 8 - len);
        }
        // 确定 奇偶校验位
        const uint32_t mask = 0x1;
        for (uint32_t bitIndex = 0; bitIndex < MESSAGE_BITS_PER_BLOCK; bitIndex++)
        {
            uint8_t isOne = (blockArray[blockIndex] >> bitIndex) & mask;
            if (1 == isOne)
            {
                uint8_t position = bitIndex + 1 + skip;
                oneBitIndex[oneCount] = position;
                oneCount++;
                if ((int)log2(position + 1) == skip)
                {
                    skip++;
                }
            }
        }
        int parityBits = multipleXor(oneBitIndex, oneCount);
        // 确定全局奇偶校验位（计算其他 bit 和奇偶性）
        for (uint32_t bitIndex = 0; bitIndex < PARITY_BITS_NUM - 1; bitIndex++)
        {
            uint8_t isOne = (parityBits >> bitIndex) & mask;
            if (1 == isOne)
            {
                oneCount++;
            }
        }
        // 保存 奇偶校验位 到 block 中
        blockArray[blockIndex] |= ((oneCount & 1) << MESSAGE_BITS_PER_BLOCK);
        for (uint32_t bitIndex = 0; bitIndex < PARITY_BITS_NUM - 1; bitIndex++)
        {
            blockArray[blockIndex] |= ((parityBits >> bitIndex) & mask) << (MESSAGE_BITS_PER_BLOCK + bitIndex + 1);
        }
        printBlock(blockArray[blockIndex]);
    }
    ptr->blockNum = blockNum;
    ptr->blockArray = blockArray;
}

/**
 * @brief 解码
 * 
 * @param input encoded data
 * @param blockNum 帧数量
 * @param ptr 暂时无用
 */
void decode(block *input, uint64_t blockNum, Block_t *ptr)
{
    char *decodedResult = (char *)malloc(sizeof(char) * (MESSAGE_BITS_PER_BLOCK / 8) * blockNum + 1);
    memset(decodedResult, 0, sizeof(char) * (MESSAGE_BITS_PER_BLOCK / 8) * blockNum + 1);
    for (uint64_t blockIndex = 0; blockIndex < blockNum; blockIndex++)
    {
        printf("On Block %d:\n", blockIndex);
        printBlock(input[blockIndex]);

        uint8_t oneCount = 0;
        uint8_t oneBitIndex[BITS_PER_BLOCK];

        const uint32_t mask = 0x1;
        uint8_t skip = 2;
        // 此处存在两种索引方式
        // 先通过 /see https://github.com/YuzuApchan/hamming-codes 快速计算误差比特的位置
        for (uint8_t bitIndex = 0; bitIndex < MESSAGE_BITS_PER_BLOCK; bitIndex++)
        {
            uint8_t position = bitIndex + 1 + skip;
            if ((int)log2(position + 1) == skip)
            {
                skip++;
            }
            if (1 == ((input[blockIndex] >> bitIndex) & mask))
            {
                oneBitIndex[oneCount] = position;
                oneCount++;
            }
        }
        uint8_t position = 1;
        for (uint8_t bitIndex = MESSAGE_BITS_PER_BLOCK + 1; bitIndex < BITS_PER_BLOCK; bitIndex++)
        {
            if (1 == ((input[blockIndex] >> bitIndex) & mask))
            {
                oneBitIndex[oneCount] = position;
                oneCount++;
            }
            position <<= 1;
        }

        uint8_t errorLocation = multipleXor(oneBitIndex, oneCount);
        // https://github.com/YuzuApchan/hamming-codes 中的索引转换到 需求里的索引方式
        if (0 != errorLocation)
        {
            if (0 == (oneCount & 1) ^ ((input[blockIndex] >> (MESSAGE_BITS_PER_BLOCK)) & mask))
            {
                printf("More than one error detected.\n");
                memset(decodedResult + blockIndex * MESSAGE_BITS_PER_BLOCK / 8,
                       '-', MESSAGE_BITS_PER_BLOCK / 8);
            }
            else
            {
                if ((int)log2(errorLocation) == log2(errorLocation))
                {
                    errorLocation = log2(errorLocation) + MESSAGE_BITS_PER_BLOCK + 1;
                }
                else
                {
                    errorLocation = errorLocation - (int)log2(errorLocation) - 1 - 1;
                }
                printf("Detected error at position %d, flipping bit.\n", errorLocation);
                input[blockIndex] = input[blockIndex] ^ (1 << errorLocation);
                printBlock(input[blockIndex]);
                memcpy(&decodedResult[blockIndex * MESSAGE_BITS_PER_BLOCK / 8],
                       &input[blockIndex], MESSAGE_BITS_PER_BLOCK / 8);
            }
        }
        else
        {
            printf("No error detected.\n");
            printBlock(input[blockIndex]);
            memcpy(&decodedResult[blockIndex * MESSAGE_BITS_PER_BLOCK / 8],
                   &input[blockIndex], MESSAGE_BITS_PER_BLOCK / 8);
        }
    }
    printf("Decoded hamming code:%x\n", decodedResult);
}

/**
 * @brief 一串异或计算误比特的索引、检验bit
 * 详见 @see https://github.com/YuzuApchan/hamming-codes
 * @param indicies
 * @param len
 * @return int
 */
int multipleXor(uint8_t *indicies, uint8_t len)
{
    if (0 == len)
    {
        return 0;
    }
    else
    {
        int val = indicies[0];
        for (int i = 1; i < len; i++)
        {
            val = val ^ indicies[i];
        }
        return val;
    }
}

/**
 * @brief 打印 block，包括二进制（大端序）和16进制
 * 
 * @param i 
 */
void printBlock(block i)
{
    printf("%08x\n", i);
    size_t size = sizeof(block) * 8;
    size_t current_bit = size;

    char *str = malloc(size + 1);
    if (!str)
        return;
    str[size] = 0;

    unsigned u = *(unsigned *)&i;
    // u >>= (sizeof(block) * 8 - BITS_PER_BLOCK);
    for (; current_bit--; u >>= 1)
        str[current_bit] = u & 1 ? '1' : '0';

    for (size_t i = 0; i < PARITY_BITS_NUM; i++)
    {
        putchar(str[i + 2]);
        putchar(' ');
    }
    printf("------ ");
    for (size_t i = PARITY_BITS_NUM; i < size - 1; i++)
    {
        putchar(str[i + 2]);
        putchar(' ');
        if ((i + 3) % 4 == 0)
        {
            putchar(' ');
        }
    }
    printf("\n");
}