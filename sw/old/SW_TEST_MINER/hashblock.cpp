#if 0
 	g++ -o hashblock hashblock.cpp -lssl -lcrypto
    exit 0
#endif

 
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

typedef long long  int64;
typedef unsigned long long  uint64;


#if 0
// http://blockexplorer.com/b/12345
#define BLOK_VER   1
#define BLOCK_PREV_HASH "0000000076876082384460fb5a231cc5a5e874b9762e15a4e7b1fc068f749cdf"
#define BLOCK_MRKL "fd1dc97a826eb93b485b6bada84a807ee81181f7ab2720cefb5fa96729363157"
#define BLOCK_TIME 1240784732
#define BLOCK_BITS 486604799
#define BLOCK_WIN_NONCE 784807199 
#endif



#if 1
// http://blockexplorer.com/b/99999
#define BLOK_VER   1
#define BLOCK_PREV_HASH "0000000000002103637910d267190996687fb095880d432c6531a527c8ec53d1"
#define BLOCK_MRKL "110ed92f558a1e3a94976ddea5c32f030670b5c58c3cc4d857ac14d7a1547a90"
#define BLOCK_TIME 1293623731
#define BLOCK_BITS 453281356
#define BLOCK_WIN_NONCE 3892545714 
#endif


#if 0
//http://blockexplorer.com/b/1
#define BLOK_VER   1
#define BLOCK_PREV_HASH "0000000000000000000000000000000000000000000000000000000000000000"
#define BLOCK_MRKL "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"
#define BLOCK_TIME 0x495FAB29
#define BLOCK_BITS 0x1D00FFFF
#define BLOCK_WIN_NONCE 0x7C2BAC1D 
#endif
 
/*
4B1E5E4A29AB5F49FFFF001D1DAC2B7C
4b1e5e4a29ab5f49ffff001d1dac2b7c
4B1E5E4A29AB5F49FFFF001D1DAC2B7C
*/
#if 0
// http://blockexplorer.com/b/233969
#define BLOK_VER   2
#define BLOCK_PREV_HASH "0000000000000096c2f9e100d4606db1b1e565da337a5905d7df510ad043d4d8"
#define BLOCK_MRKL "e83f8631a83e84b4ad889e44c182f055022071f1c7b8d9a34d56f8697c2271d8"
#define BLOCK_TIME 1367364456
#define BLOCK_BITS 436316733
#define BLOCK_WIN_NONCE 3657530658 
#endif

#if 0
//http://blockexplorer.com/b/2
#define BLOK_VER   1
#define BLOCK_PREV_HASH "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048"
#define BLOCK_MRKL "9b0fc92260312ce44e74ef369f5c66bbb85848f2eddd5a7a1cde251e54ccfdd5"
#define BLOCK_TIME 1231469744
#define BLOCK_BITS 486604799
#define BLOCK_WIN_NONCE 1639830024 
#endif

inline int Testuint256AdHoc(std::vector<std::string> vArg);



/** Base class without constructors for uint256 and uint160.
 * This makes the compiler let u use it in a union.
 */
template<unsigned int BITS>
class base_uint
{
protected:
    enum { WIDTH=BITS/32 };
    unsigned int pn[WIDTH];
public:

    bool operator!() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (pn[i] != 0)
                return false;
        return true;
    }

    const base_uint operator~() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        return ret;
    }

    const base_uint operator-() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        ret++;
        return ret;
    }

    double getdouble() const
    {
        double ret = 0.0;
        double fact = 1.0;
        for (int i = 0; i < WIDTH; i++) {
            ret += fact * pn[i];
            fact *= 4294967296.0;
        }
        return ret;
    }

    base_uint& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator^=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] ^= b.pn[i];
        return *this;
    }

    base_uint& operator&=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] &= b.pn[i];
        return *this;
    }

    base_uint& operator|=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] |= b.pn[i];
        return *this;
    }

    base_uint& operator^=(uint64 b)
    {
        pn[0] ^= (unsigned int)b;
        pn[1] ^= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator|=(uint64 b)
    {
        pn[0] |= (unsigned int)b;
        pn[1] |= (unsigned int)(b >> 32);
        return *this;
    }

    base_uint& operator<<=(unsigned int shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i+k+1 < WIDTH && shift != 0)
                pn[i+k+1] |= (a.pn[i] >> (32-shift));
            if (i+k < WIDTH)
                pn[i+k] |= (a.pn[i] << shift);
        }
        return *this;
    }

    base_uint& operator>>=(unsigned int shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i-k-1 >= 0 && shift != 0)
                pn[i-k-1] |= (a.pn[i] << (32-shift));
            if (i-k >= 0)
                pn[i-k] |= (a.pn[i] >> shift);
        }
        return *this;
    }

    base_uint& operator+=(const base_uint& b)
    {
        uint64 carry = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            uint64 n = carry + pn[i] + b.pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator-=(const base_uint& b)
    {
        *this += -b;
        return *this;
    }

    base_uint& operator+=(uint64 b64)
    {
        base_uint b;
        b = b64;
        *this += b;
        return *this;
    }

    base_uint& operator-=(uint64 b64)
    {
        base_uint b;
        b = b64;
        *this += -b;
        return *this;
    }


    base_uint& operator++()
    {
        // prefix operator
        int i = 0;
        while (++pn[i] == 0 && i < WIDTH-1)
            i++;
        return *this;
    }

    const base_uint operator++(int)
    {
        // postfix operator
        const base_uint ret = *this;
        ++(*this);
        return ret;
    }

    base_uint& operator--()
    {
        // prefix operator
        int i = 0;
        while (--pn[i] == -1 && i < WIDTH-1)
            i++;
        return *this;
    }

    const base_uint operator--(int)
    {
        // postfix operator
        const base_uint ret = *this;
        --(*this);
        return ret;
    }


    friend inline bool operator<(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator<=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator>(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator>=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator==(const base_uint& a, const base_uint& b)
    {
        for (int i = 0; i < base_uint::WIDTH; i++)
            if (a.pn[i] != b.pn[i])
                return false;
        return true;
    }

    friend inline bool operator==(const base_uint& a, uint64 b)
    {
        if (a.pn[0] != (unsigned int)b)
            return false;
        if (a.pn[1] != (unsigned int)(b >> 32))
            return false;
        for (int i = 2; i < base_uint::WIDTH; i++)
            if (a.pn[i] != 0)
                return false;
        return true;
    }

    friend inline bool operator!=(const base_uint& a, const base_uint& b)
    {
        return (!(a == b));
    }

    friend inline bool operator!=(const base_uint& a, uint64 b)
    {
        return (!(a == b));
    }



    std::string GetHex() const
    {
        char psz[sizeof(pn)*2 + 1];
        for (unsigned int i = 0; i < sizeof(pn); i++)
            sprintf(psz + i*2, "%02x", ((unsigned char*)pn)[sizeof(pn) - i - 1]);
        return std::string(psz, psz + sizeof(pn)*2);
    }

    void SetHex(const char* psz)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;

        // skip leading spaces
        while (isspace(*psz))
            psz++;

        // skip 0x
        if (psz[0] == '0' && tolower(psz[1]) == 'x')
            psz += 2;

        // hex string to uint
        static const unsigned char phexdigit[256] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0 };
        const char* pbegin = psz;
        while (phexdigit[(unsigned char)*psz] || *psz == '0')
            psz++;
        psz--;
        unsigned char* p1 = (unsigned char*)pn;
        unsigned char* pend = p1 + WIDTH * 4;
        while (psz >= pbegin && p1 < pend)
        {
            *p1 = phexdigit[(unsigned char)*psz--];
            if (psz >= pbegin)
            {
                *p1 |= (phexdigit[(unsigned char)*psz--] << 4);
                p1++;
            }
        }
    }

    void SetHex(const std::string& str)
    {
        SetHex(str.c_str());
    }

    std::string ToString() const
    {
        return (GetHex());
    }

    unsigned char* begin()
    {
        return (unsigned char*)&pn[0];
    }

    unsigned char* end()
    {
        return (unsigned char*)&pn[WIDTH];
    }

    const unsigned char* begin() const
    {
        return (unsigned char*)&pn[0];
    }

    const unsigned char* end() const
    {
        return (unsigned char*)&pn[WIDTH];
    }

    unsigned int size() const
    {
        return sizeof(pn);
    }

    uint64 Get64(int n=0) const
    {
        return pn[2*n] | (uint64)pn[2*n+1] << 32;
    }

//    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return sizeof(pn);
    }

    template<typename Stream>
//    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        s.write((char*)pn, sizeof(pn));
    }

    template<typename Stream>
//    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        s.read((char*)pn, sizeof(pn));
    }


    friend class uint160;
    friend class uint256;
    friend inline int Testuint256AdHoc(std::vector<std::string> vArg);
};

typedef base_uint<160> base_uint160;
typedef base_uint<256> base_uint256;



//
// uint160 and uint256 could be implemented as templates, but to keep
// compile errors and debugging cleaner, they're copy and pasted.
//



//////////////////////////////////////////////////////////////////////////////
//
// uint160
//

/** 160-bit unsigned integer */
class uint160 : public base_uint160
{
public:
    typedef base_uint160 basetype;

    uint160()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint160(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint160& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint160(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint160& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint160(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint160(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint160& a, uint64 b)                           { return (base_uint160)a == b; }
inline bool operator!=(const uint160& a, uint64 b)                           { return (base_uint160)a != b; }
inline const uint160 operator<<(const base_uint160& a, unsigned int shift)   { return uint160(a) <<= shift; }
inline const uint160 operator>>(const base_uint160& a, unsigned int shift)   { return uint160(a) >>= shift; }
inline const uint160 operator<<(const uint160& a, unsigned int shift)        { return uint160(a) <<= shift; }
inline const uint160 operator>>(const uint160& a, unsigned int shift)        { return uint160(a) >>= shift; }

inline const uint160 operator^(const base_uint160& a, const base_uint160& b) { return uint160(a) ^= b; }
inline const uint160 operator&(const base_uint160& a, const base_uint160& b) { return uint160(a) &= b; }
inline const uint160 operator|(const base_uint160& a, const base_uint160& b) { return uint160(a) |= b; }
inline const uint160 operator+(const base_uint160& a, const base_uint160& b) { return uint160(a) += b; }
inline const uint160 operator-(const base_uint160& a, const base_uint160& b) { return uint160(a) -= b; }

inline bool operator<(const base_uint160& a, const uint160& b)          { return (base_uint160)a <  (base_uint160)b; }
inline bool operator<=(const base_uint160& a, const uint160& b)         { return (base_uint160)a <= (base_uint160)b; }
inline bool operator>(const base_uint160& a, const uint160& b)          { return (base_uint160)a >  (base_uint160)b; }
inline bool operator>=(const base_uint160& a, const uint160& b)         { return (base_uint160)a >= (base_uint160)b; }
inline bool operator==(const base_uint160& a, const uint160& b)         { return (base_uint160)a == (base_uint160)b; }
inline bool operator!=(const base_uint160& a, const uint160& b)         { return (base_uint160)a != (base_uint160)b; }
inline const uint160 operator^(const base_uint160& a, const uint160& b) { return (base_uint160)a ^  (base_uint160)b; }
inline const uint160 operator&(const base_uint160& a, const uint160& b) { return (base_uint160)a &  (base_uint160)b; }
inline const uint160 operator|(const base_uint160& a, const uint160& b) { return (base_uint160)a |  (base_uint160)b; }
inline const uint160 operator+(const base_uint160& a, const uint160& b) { return (base_uint160)a +  (base_uint160)b; }
inline const uint160 operator-(const base_uint160& a, const uint160& b) { return (base_uint160)a -  (base_uint160)b; }

inline bool operator<(const uint160& a, const base_uint160& b)          { return (base_uint160)a <  (base_uint160)b; }
inline bool operator<=(const uint160& a, const base_uint160& b)         { return (base_uint160)a <= (base_uint160)b; }
inline bool operator>(const uint160& a, const base_uint160& b)          { return (base_uint160)a >  (base_uint160)b; }
inline bool operator>=(const uint160& a, const base_uint160& b)         { return (base_uint160)a >= (base_uint160)b; }
inline bool operator==(const uint160& a, const base_uint160& b)         { return (base_uint160)a == (base_uint160)b; }
inline bool operator!=(const uint160& a, const base_uint160& b)         { return (base_uint160)a != (base_uint160)b; }
inline const uint160 operator^(const uint160& a, const base_uint160& b) { return (base_uint160)a ^  (base_uint160)b; }
inline const uint160 operator&(const uint160& a, const base_uint160& b) { return (base_uint160)a &  (base_uint160)b; }
inline const uint160 operator|(const uint160& a, const base_uint160& b) { return (base_uint160)a |  (base_uint160)b; }
inline const uint160 operator+(const uint160& a, const base_uint160& b) { return (base_uint160)a +  (base_uint160)b; }
inline const uint160 operator-(const uint160& a, const base_uint160& b) { return (base_uint160)a -  (base_uint160)b; }

inline bool operator<(const uint160& a, const uint160& b)               { return (base_uint160)a <  (base_uint160)b; }
inline bool operator<=(const uint160& a, const uint160& b)              { return (base_uint160)a <= (base_uint160)b; }
inline bool operator>(const uint160& a, const uint160& b)               { return (base_uint160)a >  (base_uint160)b; }
inline bool operator>=(const uint160& a, const uint160& b)              { return (base_uint160)a >= (base_uint160)b; }
inline bool operator==(const uint160& a, const uint160& b)              { return (base_uint160)a == (base_uint160)b; }
inline bool operator!=(const uint160& a, const uint160& b)              { return (base_uint160)a != (base_uint160)b; }
inline const uint160 operator^(const uint160& a, const uint160& b)      { return (base_uint160)a ^  (base_uint160)b; }
inline const uint160 operator&(const uint160& a, const uint160& b)      { return (base_uint160)a &  (base_uint160)b; }
inline const uint160 operator|(const uint160& a, const uint160& b)      { return (base_uint160)a |  (base_uint160)b; }
inline const uint160 operator+(const uint160& a, const uint160& b)      { return (base_uint160)a +  (base_uint160)b; }
inline const uint160 operator-(const uint160& a, const uint160& b)      { return (base_uint160)a -  (base_uint160)b; }






//////////////////////////////////////////////////////////////////////////////
//
// uint256
//

/** 256-bit unsigned integer */
class uint256 : public base_uint256
{
public:
    typedef base_uint256 basetype;

    uint256()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint256& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint256(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256& operator=(uint64 b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint256(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint256(const std::vector<unsigned char>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint256& a, uint64 b)                           { return (base_uint256)a == b; }
inline bool operator!=(const uint256& a, uint64 b)                           { return (base_uint256)a != b; }
inline const uint256 operator<<(const base_uint256& a, unsigned int shift)   { return uint256(a) <<= shift; }
inline const uint256 operator>>(const base_uint256& a, unsigned int shift)   { return uint256(a) >>= shift; }
inline const uint256 operator<<(const uint256& a, unsigned int shift)        { return uint256(a) <<= shift; }
inline const uint256 operator>>(const uint256& a, unsigned int shift)        { return uint256(a) >>= shift; }

inline const uint256 operator^(const base_uint256& a, const base_uint256& b) { return uint256(a) ^= b; }
inline const uint256 operator&(const base_uint256& a, const base_uint256& b) { return uint256(a) &= b; }
inline const uint256 operator|(const base_uint256& a, const base_uint256& b) { return uint256(a) |= b; }
inline const uint256 operator+(const base_uint256& a, const base_uint256& b) { return uint256(a) += b; }
inline const uint256 operator-(const base_uint256& a, const base_uint256& b) { return uint256(a) -= b; }

inline bool operator<(const base_uint256& a, const uint256& b)          { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const base_uint256& a, const uint256& b)         { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const base_uint256& a, const uint256& b)          { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const base_uint256& a, const uint256& b)         { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const base_uint256& a, const uint256& b)         { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const base_uint256& a, const uint256& b)         { return (base_uint256)a != (base_uint256)b; }
inline const uint256 operator^(const base_uint256& a, const uint256& b) { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256 operator&(const base_uint256& a, const uint256& b) { return (base_uint256)a &  (base_uint256)b; }
inline const uint256 operator|(const base_uint256& a, const uint256& b) { return (base_uint256)a |  (base_uint256)b; }
inline const uint256 operator+(const base_uint256& a, const uint256& b) { return (base_uint256)a +  (base_uint256)b; }
inline const uint256 operator-(const base_uint256& a, const uint256& b) { return (base_uint256)a -  (base_uint256)b; }

inline bool operator<(const uint256& a, const base_uint256& b)          { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const uint256& a, const base_uint256& b)         { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const uint256& a, const base_uint256& b)          { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const uint256& a, const base_uint256& b)         { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const uint256& a, const base_uint256& b)         { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const uint256& a, const base_uint256& b)         { return (base_uint256)a != (base_uint256)b; }
inline const uint256 operator^(const uint256& a, const base_uint256& b) { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256 operator&(const uint256& a, const base_uint256& b) { return (base_uint256)a &  (base_uint256)b; }
inline const uint256 operator|(const uint256& a, const base_uint256& b) { return (base_uint256)a |  (base_uint256)b; }
inline const uint256 operator+(const uint256& a, const base_uint256& b) { return (base_uint256)a +  (base_uint256)b; }
inline const uint256 operator-(const uint256& a, const base_uint256& b) { return (base_uint256)a -  (base_uint256)b; }

inline bool operator<(const uint256& a, const uint256& b)               { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const uint256& a, const uint256& b)              { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const uint256& a, const uint256& b)               { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const uint256& a, const uint256& b)              { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const uint256& a, const uint256& b)              { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const uint256& a, const uint256& b)              { return (base_uint256)a != (base_uint256)b; }
inline const uint256 operator^(const uint256& a, const uint256& b)      { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256 operator&(const uint256& a, const uint256& b)      { return (base_uint256)a &  (base_uint256)b; }
inline const uint256 operator|(const uint256& a, const uint256& b)      { return (base_uint256)a |  (base_uint256)b; }
inline const uint256 operator+(const uint256& a, const uint256& b)      { return (base_uint256)a +  (base_uint256)b; }
inline const uint256 operator-(const uint256& a, const uint256& b)      { return (base_uint256)a -  (base_uint256)b; }










#ifdef TEST_UINT256

inline int Testuint256AdHoc(std::vector<std::string> vArg)
{
    uint256 g(0);


    printf("%s\n", g.ToString().c_str());
    g--;  printf("g--\n");
    printf("%s\n", g.ToString().c_str());
    g--;  printf("g--\n");
    printf("%s\n", g.ToString().c_str());
    g++;  printf("g++\n");
    printf("%s\n", g.ToString().c_str());
    g++;  printf("g++\n");
    printf("%s\n", g.ToString().c_str());
    g++;  printf("g++\n");
    printf("%s\n", g.ToString().c_str());
    g++;  printf("g++\n");
    printf("%s\n", g.ToString().c_str());



    uint256 a(7);
    printf("a=7\n");
    printf("%s\n", a.ToString().c_str());

    uint256 b;
    printf("b undefined\n");
    printf("%s\n", b.ToString().c_str());
    int c = 3;

    a = c;
    a.pn[3] = 15;
    printf("%s\n", a.ToString().c_str());
    uint256 k(c);

    a = 5;
    a.pn[3] = 15;
    printf("%s\n", a.ToString().c_str());
    b = 1;
    b <<= 52;

    a |= b;

    a ^= 0x500;

    printf("a %s\n", a.ToString().c_str());

    a = a | b | (uint256)0x1000;


    printf("a %s\n", a.ToString().c_str());
    printf("b %s\n", b.ToString().c_str());

    a = 0xfffffffe;
    a.pn[4] = 9;

    printf("%s\n", a.ToString().c_str());
    a++;
    printf("%s\n", a.ToString().c_str());
    a++;
    printf("%s\n", a.ToString().c_str());
    a++;
    printf("%s\n", a.ToString().c_str());
    a++;
    printf("%s\n", a.ToString().c_str());

    a--;
    printf("%s\n", a.ToString().c_str());
    a--;
    printf("%s\n", a.ToString().c_str());
    a--;
    printf("%s\n", a.ToString().c_str());
    uint256 d = a--;
    printf("%s\n", d.ToString().c_str());
    printf("%s\n", a.ToString().c_str());
    a--;
    printf("%s\n", a.ToString().c_str());
    a--;
    printf("%s\n", a.ToString().c_str());

    d = a;

    printf("%s\n", d.ToString().c_str());
    for (int i = uint256::WIDTH-1; i >= 0; i--) printf("%08x", d.pn[i]); printf("\n");

    uint256 neg = d;
    neg = ~neg;
    printf("%s\n", neg.ToString().c_str());


    uint256 e = uint256("0xABCDEF123abcdef12345678909832180000011111111");
    printf("\n");
    printf("%s\n", e.ToString().c_str());


    printf("\n");
    uint256 x1 = uint256("0xABCDEF123abcdef12345678909832180000011111111");
    uint256 x2;
    printf("%s\n", x1.ToString().c_str());
    for (int i = 0; i < 270; i += 4)
    {
        x2 = x1 << i;
        printf("%s\n", x2.ToString().c_str());
    }

    printf("\n");
    printf("%s\n", x1.ToString().c_str());
    for (int i = 0; i < 270; i += 4)
    {
        x2 = x1;
        x2 >>= i;
        printf("%s\n", x2.ToString().c_str());
    }


    for (int i = 0; i < 100; i++)
    {
        uint256 k = (~uint256(0) >> i);
        printf("%s\n", k.ToString().c_str());
    }

    for (int i = 0; i < 100; i++)
    {
        uint256 k = (~uint256(0) << i);
        printf("%s\n", k.ToString().c_str());
    }

    return (0);
}

#endif

#endif


 
// this is the block header, it is 80 bytes long (steal this code)
typedef struct block_header {
        unsigned int    version;
        // dont let the "char" fool you, this is binary data not the human readable version
        unsigned char   prev_block[32];
        unsigned char   merkle_root[32];
        unsigned int    timestamp;
        unsigned int    bits;
        unsigned int    nonce;
} block_header;
 
 
// we need a helper function to convert hex to binary, this function is unsafe and slow, but very readable (write something better)
void hex2bin(unsigned char* dest, const char* src)
{
        unsigned char bin;
        int c, pos;
        char buf[3];
 
        pos=0;
        c=0;
        buf[2] = 0;
        while(c < strlen(src))
        {
                // read in 2 characaters at a time
                buf[0] = src[c++];
                buf[1] = src[c++];
                // convert them to a interger and recast to a char (uint8)
                dest[pos++] = (unsigned char)strtol(buf, NULL, 16);
        }
       
}
 
// this function is mostly useless in a real implementation, were only using it for demonstration purposes
void hexdump(unsigned char* data, int len)
{
        int c;
       
        c=0;
        while(c < len)
        {
                printf("%.2x", data[c++]);
        }
        printf("\n");
}
 
// this function swaps the byte ordering of binary data, this code is slow and bloated (write your own)
void byte_swap(unsigned char* data, int len) {
        int c;
        unsigned char tmp[len];
       
        c=0;
        while(c<len)
        {
                tmp[c] = data[len-(c+1)];
                c++;
        }
       
        c=0;
        while(c<len)
        {
                data[c] = tmp[c];
                c++;
        }
}

void FormatHashBuffers(char* pmidstate, char* pdata, char* phash1);

 
int main() {
        // start with a block header struct
        block_header header;
       
        // we need a place to store the checksums
        unsigned char hash1[SHA256_DIGEST_LENGTH];
        unsigned char hash2[SHA256_DIGEST_LENGTH];
       
        // you should be able to reuse these, but openssl sha256 is slow, so your probbally not going to implement this anyway
    SHA256_CTX sha256_pass1, sha256_pass2;
 

        // we are going to supply the block header with the values from the generation block 0
        header.version =        BLOK_VER;
        hex2bin(header.prev_block,             BLOCK_PREV_HASH);
        hex2bin(header.merkle_root,            BLOCK_MRKL);
        header.timestamp =      BLOCK_TIME;
	header.bits =           BLOCK_BITS;
        header.nonce =          BLOCK_WIN_NONCE;
       

        // the endianess of the checksums needs to be little, this swaps them form the big endian format you normally see in block explorer
/*
	byte_swap((unsigned char*)&header.version, 4);
	byte_swap((unsigned char*)&header.timestamp, 4);
	byte_swap((unsigned char*)&header.bits, 4);
 	byte_swap((unsigned char*)&header.nonce, 4);       
*/
        byte_swap(header.prev_block, 32);
        byte_swap(header.merkle_root, 32);
       
        // dump out some debug data to the terminal

        printf("\n\nsizeof(block_header) = %x\n", (unsigned int)sizeof(block_header));
        printf("Block header (in human readable hexadecimal representation): ");
        hexdump((unsigned char*)&header, (unsigned int)sizeof(block_header));
printf("\n\n");
        // Use SSL's sha256 functions, it needs to be initialized
    SHA256_Init(&sha256_pass1);
    // then you 'can' feed data to it in chuncks, but here were just making one pass cause the data is so small
    //printf("STARTSTATE: ");
    //hexdump((unsigned char*)sha256_pass1.h, SHA256_DIGEST_LENGTH);
    SHA256_Update(&sha256_pass1, ((unsigned char*)&header), 64 + 16);

    // this ends the sha256 session and writes the checksum to hash1
    SHA256_Final(hash1, &sha256_pass1);
       
    // to display this, we want to swap the byte order to big endian
    byte_swap(hash1, SHA256_DIGEST_LENGTH);
    printf("Useless First Pass Checksum: ");
    hexdump(hash1, SHA256_DIGEST_LENGTH);
 
        
	

        // but to calculate the checksum again, we need it in little endian, so swap it back
        byte_swap(hash1, SHA256_DIGEST_LENGTH);
       
    //same as above
    SHA256_Init(&sha256_pass2);
    SHA256_Update(&sha256_pass2, hash1, SHA256_DIGEST_LENGTH);
    SHA256_Final(hash2, &sha256_pass2);
       
        byte_swap(hash2, SHA256_DIGEST_LENGTH);
        printf("!!!!!VER1 ");
        hexdump(hash2, SHA256_DIGEST_LENGTH);


	char phash1[512];
	char pdata[512];
	char pmidstate[512];
	 FormatHashBuffers(pmidstate,pdata, phash1);

 
        return 0;
}















typedef unsigned int uint32_t; 


inline uint32_t ByteReverse(uint32_t value)
{
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return (value<<16) | (value>>16);
}






int static FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*)pbuffer;
    unsigned int blocks = 1 + ((len + 8) / 64);
    unsigned char* pend = pdata + 64 * blocks;
    memset(pdata + len, 0, 64 * blocks - len);
    pdata[len] = 0x80;
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return blocks;
}



int SWAP32( int x) {
	return (((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) >> 24) & 0xff);
}


static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};


void SHA256Transform(void* pstate, void* pinput, const void* pinit)
{
    SHA256_CTX ctx;
    unsigned char data[64];

    SHA256_Init(&ctx);

    for (int i = 0; i < 16; i++)
        ((uint32_t*)data)[i] = ByteReverse(((uint32_t*)pinput)[i]);

    for (int i = 0; i < 8; i++)
        ctx.h[i] = ((uint32_t*)pinit)[i];

    SHA256_Update(&ctx, data, sizeof(data));
    for (int i = 0; i < 8; i++)
        ((uint32_t*)pstate)[i] = ctx.h[i];
}




void FormatHashBuffers(char* pmidstate, char* pdata, char* phash1)
{
    //
    // Pre-build hash buffers
    //
    struct
    {
        struct unnamed2
        {
            int nVersion;
            uint256 hashPrevBlock;
            uint256 hashMerkleRoot;
            unsigned int nTime;
            unsigned int nBits;
            unsigned int nNonce;
        }
        block;
        unsigned char pchPadding0[64];
        uint256 hash1;
        unsigned char pchPadding1[64];
    }
    tmp;
    memset(&tmp, 0, sizeof(tmp));




    tmp.block.nVersion       = BLOK_VER;
    //tmp.block.hashPrevBlock  = pblock->hashPrevBlock;
    //tmp.block.hashMerkleRoot = pblock->hashMerkleRoot;
    tmp.block.nTime          = BLOCK_TIME;
    tmp.block.nBits          = BLOCK_BITS;
    tmp.block.nNonce         = BLOCK_WIN_NONCE;

//	tmp.block.hashPrevBlock= std::string("0000000000000000000000000000000000000000000000000000000000000000");
//  tmp.block.hashMerkleRoot=string("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
	hex2bin((unsigned char*)(void*)&tmp.block.hashPrevBlock,              BLOCK_PREV_HASH);
    hex2bin((unsigned char*)(void*)&tmp.block.hashMerkleRoot,             BLOCK_MRKL);
     	   
	byte_swap((unsigned char*)(void*)&tmp.block.hashPrevBlock, 32);
	byte_swap((unsigned char*)(void*)&tmp.block.hashMerkleRoot, 32);


    FormatHashBlocks(&tmp.block, sizeof(tmp.block));
    FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));

    // Byte swap all the input buffer
    for (unsigned int i = 0; i < sizeof(tmp)/4; i++)
        ((unsigned int*)&tmp)[i] = ByteReverse(((unsigned int*)&tmp)[i]);

    // Precalc the first half of the first hash, which stays constant
    /*printf("::::\n");
    hexdump((unsigned char*)&tmp.block, 64*2);
    printf(":::::\n");
   */
    SHA256Transform(pmidstate, &tmp.block, pSHA256InitState);

    memcpy(pdata, &tmp.block, 128);
    memcpy(phash1, &tmp.hash1, 64);

	 printf("MIDSTATE:\n");
         //hexdump((unsigned char*)pmidstate, SHA256_DIGEST_LENGTH);
	 /*for (int i = 0; i < 8; i++)
		  ((uint32_t*)pmidstate)[i] = ByteReverse(((uint32_t*)pmidstate)[i]);*/
	// byte_swap((unsigned char*)pmidstate, 32);
	hexdump((unsigned char*)pmidstate, SHA256_DIGEST_LENGTH);

	char phash[64];
	pdata = (char*)&tmp.block;
	pdata += 64;
	
	/*printf("Target !!! PDATA\n");
	hexdump((unsigned char*)pdata, 64);*/
	SHA256Transform(phash1, pdata, pmidstate);
	SHA256Transform(phash, phash1, pSHA256InitState);
	
	for (int i = 0; i < 8; i++)
		  ((uint32_t*)phash)[i] = ByteReverse(((uint32_t*)phash)[i]);

	byte_swap((unsigned char*)phash, SHA256_DIGEST_LENGTH); printf("!!!!!VER2 ");
	hexdump((unsigned char*)phash, SHA256_DIGEST_LENGTH);

	 printf("COMMAND (%x):\n", BLOCK_WIN_NONCE);

 	 //printf("_ REG 000000b0 %08x\n", SWAP32(BLOCK_WIN_NONCE - 0x1000));
	 //printf("_ REG 000000c0 %08x\n", SWAP32(BLOCK_WIN_NONCE + 0x1000));
	 //printf("_ TARGET_SET 0000000100000000000000000000000000000000000000000000000100000000\n");
	 printf("_ DO_WORK B64:%08x%08x%08x M:",
		  *(((unsigned int*)(void*)&tmp.block.hashMerkleRoot) + 7), SWAP32(BLOCK_TIME), SWAP32(BLOCK_BITS)
		  );
	hexdump((unsigned char*)pmidstate, SHA256_DIGEST_LENGTH);

		
}










