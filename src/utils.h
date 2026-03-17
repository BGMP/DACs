void removeBits(unsigned int* bitarray, int size, int pos, int length) {
    unsigned int index = pos / W;
    unsigned int offset = pos % W;
    unsigned int upperbits, lowerbits, lwt, upt, mov = offset + length;

    unsigned int lb = pos + length;
    unsigned int findex = lb / W;
    unsigned int foffset = lb % W, woffset;

    if (offset + length < W) {
        upperbits = bitarray[index] >> (offset + length);
        bitarray[index] = bitarray[index] % (1 << offset);
        bitarray[index] += upperbits << offset;
        woffset = W - length;
    }
    else {
        if (offset > foffset) {
            bitarray[findex] = bitarray[findex] >> foffset;
            upperbits = bitarray[findex] % (1 << (W - offset));
            bitarray[index] = bitarray[index] % (1 << offset);
            bitarray[index] += upperbits << offset;
            index++;
            if (index < size) {
                bitarray[index] = bitarray[findex] >> (W - offset);
            }
            woffset = offset - foffset;
        }
        else {
            upperbits = bitarray[findex] >> (offset + length);
            bitarray[index] = bitarray[index] % (1 << offset);
            bitarray[index] += upperbits << offset;
            woffset = offset + (W - foffset);
        }
    }

    for (int i = findex + 1; i < size; i++) {
        if (index >= size) break;

        upperbits = bitarray[i] % (1 << (W - woffset));
        bitarray[index] += upperbits << woffset;
        index++;

        if (index >= size) break;

        bitarray[index] = bitarray[i] >> (W - woffset);
    }
}

void insertBits(unsigned int* bitarray, unsigned int size, unsigned int pos, unsigned int length) {
    unsigned int index = pos / W; // Index of the first element to modify
    unsigned int offset = pos % W; // Offset within the first element
    unsigned int upperbits, lowerbits,lwt,upt,mov=offset+length;

    upperbits = bitarray[index]>>offset;
    lowerbits = bitarray[index]%(1<<offset);

    unsigned int lb = size*W - length;
    unsigned int findex = lb/W;
    unsigned int foffset = lb % W;
    unsigned int mindex = (offset+length)/W+index;

    //printf("findex %u index %u foffset %u size %u\n",findex,index,foffset,size);

    if(findex>index){
        for(unsigned int i = 0; i<findex-index;i++){
            lwt = bitarray[findex-i]%(1<<foffset);
            upt = bitarray[findex-i-1]>>foffset;
            bitarray[size-1-i] = lwt<<(W-foffset);
            bitarray[size-1-i] += upt;
        }
    }
//    for(int i = index;i<mindex;i++) bitarray[i] = 0;
    bitarray[index]=0;
    if(foffset>offset){
        bitarray[mindex] = 0;
        bitarray[mindex] += upperbits<<(mov%W);
    }
    bitarray[index] += lowerbits;
}

void printBitArray(unsigned int* bitarray, int size) {
    int dsize = size/W+1;
    int fakesize = dsize*W - size;
    for (int i = dsize-1; i >= 0; i--) {
        for (int j = W - 1; j >= 0; j--) {
            int x = (bitarray[i] >> j) & 1;
            if(fakesize > 0) fakesize--;
            else printf("%d", x);
        }
        printf(" ");
    }
    printf("\n");
}

void printArray(unsigned int bitarray) {
    for (int j = W - 1; j >= 0; j--) {
        printf("%d", (bitarray >> j) & 1);
    }
    printf("\n");
}
