#include "decrypt.h"
#include "hex-type.h"
#include "key-type.h"

Decrypt::Decrypt (HexType in, std::vector<HexType> ks)
{
    cipherText = (HexType*) malloc (1 * sizeof (HexType)); 
    *cipherText = in;
    keySchedule = ks;    
}

HexType
Decrypt::KeyAdd (HexType words_in, HexType key)
{
    //null terminator
    size_t outLength = 32;

    Words out;
    out.w = (uint8_t *)malloc (outLength * sizeof (uint8_t));
    out.l = outLength;

    Words in = (words_in.GetHexWords());
    Words k = (key.GetHexWords());

    for (int i = 0; i < 32; i++)
    {
        uint8_t a = in.w[i];
        uint8_t b = k.w[i];
        uint8_t c = a ^ b;
        out.w[i] =  c;
    }

    HexType rt = HexType(out);
    return rt;
}

HexType 
Decrypt::RotateBlock (HexType words_in)
{
    Words in = words_in.GetHexWords();
    size_t len = in.l;

    Words out;
    out.w = (uint8_t *) malloc (len * sizeof (uint8_t));
    out.l = len;

    for (int j = 0, i = 0; j < 8; j+=2)
    {
        for (int k =0; k < 32; i+=2, k+=8)
        {
            out.w[i] = in.w[k+j];
            out.w[i+1] = in.w[k+j+1];
        }
    }

    return out;
}

HexType
Decrypt::InvShiftRows (HexType words_in)
{
    Words out;

    Words in = words_in.GetHexWords();
    size_t len = in.l;

    out.w = (uint8_t *)malloc (len * sizeof(uint8_t));
    out.l = len;

    out.w[0] = in.w[0];
    out.w[1] = in.w[1];
    out.w[2] = in.w[2];
    out.w[3] = in.w[3];
    out.w[4] = in.w[4];
    out.w[5] = in.w[5];
    out.w[6] = in.w[6];
    out.w[7] = in.w[7];

    out.w[8] = in.w[14];
    out.w[9] = in.w[15];
    out.w[10] = in.w[8];
    out.w[11] = in.w[9];
    out.w[12] = in.w[10];
    out.w[13] = in.w[11];
    out.w[14] = in.w[12];
    out.w[15] = in.w[13];

    out.w[16] = in.w[20];
    out.w[17] = in.w[21];
    out.w[18] = in.w[22];
    out.w[19] = in.w[23];
    out.w[20] = in.w[16];
    out.w[21] = in.w[17];
    out.w[22] = in.w[18];
    out.w[23] = in.w[19];

    out.w[24] = in.w[26];
    out.w[25] = in.w[27];
    out.w[26] = in.w[28];
    out.w[27] = in.w[29];
    out.w[28] = in.w[30];
    out.w[29] = in.w[31];
    out.w[30] = in.w[24];
    out.w[31] = in.w[25];

    return out;
}

HexType
Decrypt::InvByteSub (HexType words_in)
{
    Words in = words_in.GetHexWords();
    size_t len = in.l;

    Words out;
    out.w = (uint8_t *) malloc ((len+1) * sizeof(uint8_t));
    out.l = len;
    out.w[len] = '\0';

    for (size_t i = 0; i < len; i+=2)
    {
        uint8_t x = in.w[i];
        uint8_t y = in.w[i+1];

        uint8_t sub = InverseSubBox(x,y);

        out.w[i] = (sub & 0xF0) >> 4;
        out.w[i+1] = sub & 0x0F;
    }

    HexType rt = HexType(out);

    return rt;
}

HexType
Decrypt::InvMixCol (HexType words_in)
{

    uint8_t inverseMatrix[4][4]=
    {
        {0x0E, 0x0B, 0x0D, 0x09},
        {0x09, 0x0E, 0x0B, 0x0D},
        {0x0D, 0x09, 0x0E, 0x0B},
        {0x0B, 0x0D, 0x09, 0x0E} 
    };

    uint8_t *c = (uint8_t *) malloc (32 * sizeof (uint8_t));

    Words in = words_in.GetHexWords();

    uint8_t tmp[4];

    for (int vc = 0, j = 0, wj = 0, fm = 0; vc <32 ; wj += 8, fm++)
    {   
        for (int i = 0; i < 8; i+=2, vc += 2)
        {   
            int t = 0;
            j = wj;

            for (int k = 0; k < 32; k+=8, t++, j+=2)
            {   
                uint8_t x = (in.w[k+i]);
                uint8_t y = (in.w[k+i+1]);

                uint8_t l = L_Table(x,y);

                uint8_t x1 = (inverseMatrix[fm][t] & 0xF0) >> 4;
                uint8_t y1 = inverseMatrix[fm][t] & 0x0F;

                uint8_t l1 = L_Table(x1,y1);

                uint8_t max = (l + l1) > 255 ? (l + l1) - 255: (l+l1);

                int x2 = (max & 0xF0) >> 4;
                int y2 = (max & 0x0F);

                uint8_t e = E_Table(x2,y2);

                tmp[t] = x==0 && y==0 ? 0:e;
            }

            uint8_t d = tmp[0] ^ tmp[1] ^ tmp[2] ^ tmp[3];
            c[vc] = (d & 0xF0) >> 4;
            c[vc+1] = (d & 0x0F);
        }
    }

    HexType out(c,32);

    return out;
}

void
Decrypt::CreatePlainText (void)
{
    int round = 10;
    
    HexType r = KeyAdd(*cipherText,keySchedule[round]);
    r = RotateBlock (r);
    r = InvShiftRows (r);
    r = InvByteSub (r);
    
    round--;

    for (; round > 0; round--)
    {
        r = RotateBlock (r);
        r = KeyAdd(r,keySchedule[round]);
        r = RotateBlock (r);
        r = InvMixCol (r);
        r = InvShiftRows (r);
        r = InvByteSub (r);
    }
     
    r = RotateBlock (r);
    r = KeyAdd(r, keySchedule[0]);

    plainText = (HexType*) malloc (1 * sizeof(HexType));
    *plainText = r;
}

void
Decrypt::Print(void)
{
    plainText->Print();
}

HexType
Decrypt::GetPlainText (void)
{
    return (*plainText);
}
