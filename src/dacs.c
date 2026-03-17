#include "dacs.h"
//#include "utils.h"
#define FACT_RANK 20

ushort* optimizationk(uint* list, int listLength, int* nkvalues) {
	int t, i, m, k;
	uint maxInt = 0;
	for (i = 0; i < listLength; i++) {
		if (maxInt < list[i]) maxInt = list[i];
	}
	uint nBits = bits(maxInt);
	if (nBits > 0) nBits--;
	uint tamAux = nBits + 2;

	/* Build acumFreq directly without the weight array */
	uint* acumFreq = (uint*)malloc(sizeof(uint) * tamAux);

	for (uint b = 0; b < tamAux; b++) {
		uint count = 0;

		if (b >= 32) {
			count = listLength;
		}
		else {
			uint threshold = (1U << b);
			for (i = 0; i < listLength; i++) {
				if (list[i] < threshold) count++;
			}
		}
		acumFreq[b] = count;
	}

	long* s = (long*)malloc(sizeof(long) * (nBits + 1));
	uint* l = (uint*)malloc(sizeof(uint) * (nBits + 1));
	uint* b = (uint*)malloc(sizeof(uint*) * (nBits + 1));
	ulong currentSize;
	m = nBits;
	s[m] = 0;
	l[m] = 0;
	b[m] = 0;
	uint* fc = acumFreq;
	ulong minSize;
	uint minPos;

	for (t = m; t >= 0; t--) {
		minSize = -1;
		minPos = m;
		for (i = m; i >= t + 1; i--) {
			uint data_bits = (fc[m + 1] - fc[t]) * (i - t + 1);
			uint data_words = (data_bits + W - 1) / W;
			uint bitmap_bits = fc[m + 1] - fc[t];
			uint bitmap_words = (bitmap_bits + W - 1) / W;
			currentSize = s[i] + data_words * W + bitmap_words * W / FACT_RANK;

			if (minSize > currentSize) {
				minSize = currentSize;
				minPos = i;
			}
		}

		/* Never allow single level with >= 32 bits */
		uint single_level_bits = (fc[m + 1] - fc[t]) * (m - t + 1);
		uint single_level_words = (single_level_bits + W - 1) / W;
		uint single_level_cost = single_level_words * W;

		if (minSize < single_level_cost || (m - t + 1) >= 32) {
			// Use multi-level when it's better OR when single level would need >= 32 bits
			s[t] = minSize;
			l[t] = l[minPos] + 1;
			b[t] = minPos - t;
		}
		else {
			// Single level is better and fits in < 32 bits
			s[t] = single_level_cost;
			l[t] = 1;
			b[t] = m - t + 1;
		}
	}

	int L = l[0];
	ushort* kvalues = (ushort*)malloc(sizeof(ushort) * L);
	t = 0;
	for (k = 0; k < L; k++) {
		kvalues[k] = b[t];
		t = t + b[t];
	}
	free(acumFreq);
	free(l);
	free(b);
	free(s);
	(*nkvalues) = L;
	return kvalues;
}

FTRep* createFT(uint* list, uint listLength) {
	FTRep* rep = (FTRep*)malloc(sizeof(struct sFTRep));
	uint* levelSizeAux;
	uint* cont;
	uint* contB;
	ushort* kvalues;
	uint nkvalues;
	rep->listLength = listLength;
	register uint i;
	int j, k;
	uint value, newvalue;

	kvalues = optimizationk(list, listLength, &nkvalues);
	uint kval;
	uint oldval = 0;
	uint newval = 0;
	i = 0;
	uint multval = 1;

	do {
		oldval = newval;
		if (i >= nkvalues) {
			kval = 1 << (kvalues[nkvalues - 1]);
		}
		else {
			kval = 1 << (kvalues[i]);
		}	
		multval *= kval;
		newval = oldval + multval;
		i++;
	} while (oldval < newval);

	uint tamtablebase = i;
	uint* tablebase = (uint*)malloc(sizeof(uint) * tamtablebase);
	levelSizeAux = (uint*)malloc(sizeof(uint) * tamtablebase);
	cont = (uint*)malloc(sizeof(uint) * tamtablebase);
	contB = (uint*)malloc(sizeof(uint) * tamtablebase);
	oldval = 0;
	newval = 0;
	multval = 1;

	for (i = 0; i < tamtablebase; i++) {
		oldval = newval;
		if (i >= nkvalues) {
			kval = 1 << (kvalues[nkvalues - 1]);
		}
		else {
			kval = 1 << (kvalues[i]);
		}	
		multval *= kval;
		newval = oldval + multval;
		tablebase[i] = oldval;
	}

	for (i = 0; i < tamtablebase; i++) {
		levelSizeAux[i] = 0;
	}

	for (i = 0; i < listLength; i++) {
		value = list[i];
		for (j = 0; j < tamtablebase; j++) {
			if (value >= tablebase[j]) {
				levelSizeAux[j]++;
			}
		}
	}

	j = 0;
	while ((j < tamtablebase) && (levelSizeAux[j] != 0)) {
		j++;
	}
	rep->nLevels = j;
	rep->base = (uint*)malloc(sizeof(uint) * rep->nLevels);
	rep->base_bits = (ushort*)malloc(sizeof(ushort) * rep->nLevels);

	for (i = 0; i < rep->nLevels; i++) {
		if (i >= nkvalues) {
			rep->base[i] = 1 << (kvalues[nkvalues - 1]);
			rep->base_bits[i] = kvalues[nkvalues - 1];
		}
		else {
			rep->base[i] = 1 << (kvalues[i]);
			rep->base_bits[i] = kvalues[i];
		}
	}

	uint indexLevel = 0;
	for (j = 0; j < rep->nLevels; j++) {
		cont[j] = 0;
		contB[j] = 0;
	}

	rep->levels = (uint**)malloc(sizeof(uint*) * rep->nLevels);
	for (i = 0; i < rep->nLevels; i++) {
		uint size_needed = (levelSizeAux[i] * rep->base_bits[i] + W - 1) / W;
		rep->levels[i] = (uint*)malloc(sizeof(uint) * size_needed);
	}

	uint size = sizeof(uint*) * (rep->nLevels);
	uint** bits_BS = (uint**)malloc(size);
	for (i = 0; i < j - 1; i++) {
		uint bitmap_words = (levelSizeAux[i] + W - 1) / W;
		bits_BS[i] = (uint*)calloc(bitmap_words, sizeof(uint));
	}

	for (i = 0; i < listLength; i++) {
		value = list[i];
		j = rep->nLevels - 1;
		while (j >= 0) {
			if (value >= tablebase[j]) {
				newvalue = value - tablebase[j];
				for (k = 0; k < j; k++) {
					bitwrite(rep->levels[k], cont[k], rep->base_bits[k], newvalue % rep->base[k]);
					cont[k] += rep->base_bits[k];
					contB[k]++;
					newvalue = newvalue / rep->base[k];
				}
				k = j;
				bitwrite(rep->levels[j], cont[j], rep->base_bits[j], newvalue % rep->base[j]);
				cont[j] += rep->base_bits[j];
				contB[j]++;
				if (j < rep->nLevels - 1) {
					bitset(bits_BS[j], contB[j] - 1);
				}
				break;
			}
			j--;
		}
	}

	size = (sizeof(bitRankW32Int*)) * (rep->nLevels - 1);
	rep->bS = (bitRankW32Int**)malloc(size);
	for (i = 0; i < rep->nLevels - 1; i++) {
		rep->bS[i] = createBitRankW32Int(bits_BS[i], levelSizeAux[i], 1, 20);
	}

	rep->sizeLastLevel = levelSizeAux[i];

	free(cont);
	free(contB);
	free(levelSizeAux);
	free(kvalues);
	free(tablebase);
	free(bits_BS);

	return rep;
}

uint accessFT(FTRep* rep, uint param) {
	uint mult = 0;
	register uint j;
	uint partialSum = 0;
	uint ini = param - 1;
	uint nLevels = rep->nLevels;
	uint** level;
	uint readByte;
	uint cont, pos, rankini;
	partialSum = 0;
	j = 0;
	level = rep->levels;
	pos = ini;
	mult = 0;
	cont = ini * rep->base_bits[j];

	readByte = bitread(level[j], cont, rep->base_bits[j]);

	if (nLevels == 1) {
		return readByte;
	}

	while ((!bitget(rep->bS[j]->data, pos))) {
		rankini = rank(rep->bS[j], ini - 1);

		ini = ini - rankini;
		partialSum = partialSum + (readByte << mult);

		mult += rep->base_bits[j];
		j++;
		cont = ini * rep->base_bits[j];
		pos = ini;
		readByte = bitread(level[j], cont, rep->base_bits[j]);
		readByte++;

		if (j == nLevels - 1) {
			break;
		}
	}

	partialSum = partialSum + (readByte << mult);

	return partialSum;
}

uint * decompressFT(FTRep * rep, uint n){
    uint mult=0;
    register uint i;
    register uint j;
    uint partialSum=0;
    uint nLevels=rep->nLevels;
    uint ** level=rep->levels;
    uint readByte;
    uint * list = (uint *) malloc(sizeof(uint)*n);
    uint * cont = (uint *) malloc(sizeof(byte*)*rep->nLevels);
    uint * pos = (uint *) malloc(sizeof(uint)*rep->nLevels);

 	for(j=0;j<nLevels;j++){
		cont[j]=0;
		pos[j]=0;
	}

	for(i=0;i<n;i++){
		partialSum=0;
		j=0;			
		mult=0;
		readByte = bitread(level[j],cont[j],rep->base_bits[j]);
		cont[j]+=rep->base_bits[j];
        if(j<nLevels-1)
	        while((!bitget(rep->bS[j]->data,pos[j]))){
		        pos[j]++;
		        partialSum = partialSum+(readByte<<mult);
		        mult+=rep->base_bits[j];
		        j++;
		        readByte = bitread(level[j],cont[j],rep->base_bits[j]);
                readByte++;
		        cont[j]+=rep->base_bits[j];
		        if(j==nLevels-1) break;
	        }
		if(j<nLevels-1) pos[j]++;
		partialSum = partialSum + (readByte<<mult);// + rep->tablebase[j];
		list[i]=partialSum;
	}
	free(cont);
	free(pos);
	return list;
}

void destroyFT(FTRep * rep){
	for(uint i=0;i<rep->nLevels;i++) free(rep->levels[i]);
	free(rep->levels);
	for(uint i=0;i<rep->nLevels-1;i++) destroyBitRankW32Int(rep->bS[i]);
    free(rep->bS);
	free(rep->base);
	free(rep->base_bits);
	free(rep);
}

void saveFT(FTRep * rep, char * filename){
	FILE * flist;
	flist = fopen(filename,"w");
	fwrite(&(rep->sizeLastLevel),sizeof(uint),1,flist);
	fwrite(&(rep->listLength),sizeof(uint),1,flist);
	fwrite(&(rep->nLevels),sizeof(byte),1,flist);
	fwrite(rep->base_bits,sizeof(ushort),rep->nLevels,flist);
	fwrite(rep->base,sizeof(uint),rep->nLevels,flist);
    for(int i=0;i<rep->nLevels-1;i++) save(rep->bS[i],flist);    
    uint size;
	for(int i=0;i<rep->nLevels-1;i++){
        size = rep->bS[i]->n*rep->base_bits[i];
		fwrite(rep->levels[i], sizeof(uint), (size + W - 1) / W, flist);
    }
    size = rep->sizeLastLevel*rep->base_bits[rep->nLevels-1];
	fwrite(rep->levels[rep->nLevels - 1], sizeof(uint), (size + W - 1) / W, flist);
	fclose(flist);		
}

FTRep* loadFT(char * filename){
	FILE * flist;
	FTRep * rep = (FTRep *) malloc(sizeof(struct sFTRep));
	flist = fopen(filename,"r");
	fread(&(rep->sizeLastLevel),sizeof(uint),1,flist);
	fread(&(rep->listLength),sizeof(uint),1,flist);
	fread(&(rep->nLevels),sizeof(byte),1,flist);
	
	rep->base_bits = (ushort *) malloc(sizeof(ushort)*rep->nLevels);
	fread(rep->base_bits,sizeof(ushort),rep->nLevels,flist);
		
	rep->base = (uint *) malloc(sizeof(uint)*rep->nLevels);
	fread(rep->base,sizeof(uint),rep->nLevels,flist);

    uint size = (sizeof(bitRankW32Int *))*(rep->nLevels-1);
    rep->bS = (bitRankW32Int **) malloc(size);
    for(int i=0;i<rep->nLevels-1;i++){
        rep->bS[i] = (bitRankW32Int *) malloc(sizeof(struct sbitRankW32Int));
    	load(rep->bS[i],flist);
    }

    rep->levels = (uint **) malloc(sizeof(uint *)*rep->nLevels);
    for(int i=0;i<rep->nLevels-1;i++){
		size = (rep->bS[i]->n * rep->base_bits[i] + W - 1) / W;
        rep->levels[i] = (uint *) malloc(sizeof(uint)*size);
    	fread(rep->levels[i],sizeof(uint),size,flist);
    }
	size = (rep->sizeLastLevel * rep->base_bits[rep->nLevels - 1] + W - 1) / W;
    rep->levels[rep->nLevels-1] = (uint *) malloc(sizeof(uint)*size);
	fread(rep->levels[rep->nLevels-1],sizeof(uint),size,flist);
	
	fclose(flist);
	
	return rep;
}

void printFT(FTRep * rep, int breve){
        printf("sizeLastLevel = %u\n",rep->sizeLastLevel);
        printf("ListLength = %u\n",rep->listLength);
        uint nL = rep->nLevels;
        printf("nLevels = %u\n",nL);
        printf("Base Bits = ");
        for(int ii=0;ii<nL;ii++){
            printf("%u ",rep->base_bits[ii]);
        }
        printf("\n");
        printf("Base = ");
        for(int ii=0;ii<nL;ii++){
            printf("%u ",rep->base[ii]);
        }
        printf("\n");
        if(breve!=1){
            for(int ii=0;ii<rep->nLevels-1;ii++){
                printf("Levels[%d] = ",ii);
                printBitArray(rep->levels[ii],rep->bS[ii]->n*rep->base_bits[ii]);
            }
            printf("Levels[%d] = ",rep->nLevels-1);
            printBitArray(rep->levels[rep->nLevels-1],rep->sizeLastLevel*rep->base_bits[rep->nLevels-1]);
		    for(int ii=0;ii<rep->nLevels-1;ii++){
                printf("Bs[%u] = ",ii);
                printBitArray(rep->bS[ii]->data,rep->bS[ii]->n);
            }
		}
        printf("FIN\n");
}

//----------------------dinamicas-------------------

void appendFT(FTRep* rep, uint value) {
	uint imax, i, k;
	uint rest = 0;

	// Calculate which level this value needs
	uint threshold_at_imax = 0;
	{
		imax = 0;
		uint threshold = 0;
		uint capacity = rep->base[0];

		for (uint j = 1; j <= rep->nLevels; j++) {
			threshold += capacity;
			if (value >= threshold) {
				imax = j;
				threshold_at_imax = threshold;
			}
			if (j < rep->nLevels) {
				uint old_capacity = capacity;
				capacity *= rep->base[j];
				if (capacity == 0 || capacity < old_capacity) {
					break;
				}
			}
		}
	}

	// Calculate rest if we need a new level
	if (imax >= rep->nLevels) {
		uint bits_needed = value > 0 ? bits(value) : 1;
		uint bits_covered = 0;
		for (uint i = 0; i < rep->nLevels; i++) {
			bits_covered += rep->base_bits[i];
		}
		rest = bits_needed > bits_covered ? (bits_needed - bits_covered) : 1;
	}

	// Create new level if needed
	if (imax >= rep->nLevels) {
		uint old_nlev = rep->nLevels;
		uint old_sizeLastLevel = rep->sizeLastLevel;

		rep->nLevels++;
		rep->base_bits = (ushort*)realloc(rep->base_bits, sizeof(ushort) * rep->nLevels);
		rep->base = (uint*)realloc(rep->base, sizeof(uint) * rep->nLevels);
		rep->base_bits[old_nlev] = rest;
		rep->base[old_nlev] = 1 << rest;

		uint size = (sizeof(bitRankW32Int*)) * (rep->nLevels - 1);
		rep->bS = (bitRankW32Int**)realloc(rep->bS, size);

		// Create bitmap ONLY for OLD elements
		uint words_for_bitmap = (old_sizeLastLevel + W - 1) / W;
		if (words_for_bitmap == 0) words_for_bitmap = 1;

		uint* auxLL = (uint*)calloc(words_for_bitmap, sizeof(uint));

		for (uint ii = 0; ii < old_sizeLastLevel / W; ii++) {
			auxLL[ii] = UINT_MAX;
		}

		uint remainder = old_sizeLastLevel % W;
		if (remainder > 0) {
			auxLL[old_sizeLastLevel / W] = (1U << remainder) - 1;
		}

		rep->bS[old_nlev - 1] = createBitRankW32Int(auxLL, old_sizeLastLevel, 1, FACT_RANK);
		rep->sizeLastLevel = 0;

		size = sizeof(uint*) * rep->nLevels;
		rep->levels = (uint**)realloc(rep->levels, size);

		// Allocate proper initial size for new level
		uint initial_words = (rest + W - 1) / W;
		if (initial_words == 0) initial_words = 1;
		rep->levels[old_nlev] = (uint*)calloc(initial_words, sizeof(uint));

		// Recalculate imax with the new structure
		imax = 0;
		threshold_at_imax = 0;
		uint threshold = 0;
		uint capacity = rep->base[0];
		for (uint j = 1; j <= rep->nLevels; j++) {
			threshold += capacity;
			if (value >= threshold) {
				imax = j;
				threshold_at_imax = threshold;
			}
			if (j < rep->nLevels) {
				uint old_capacity = capacity;
				capacity *= rep->base[j];
				if (capacity == 0 || capacity < old_capacity) {
					break;
				}
			}
		}
	}

	// Write chunks to levels
	uint newvalue, pos;
	uint j = imax;

	while (j >= 0) {
		if (value >= threshold_at_imax) {
			newvalue = value - threshold_at_imax;

			// Write to all levels before termination level
			for (k = 0; k < j; k++) {
				if (k < rep->nLevels - 1) {
					pos = rep->bS[k]->n;
				}
				else {
					pos = rep->sizeLastLevel;
				}

				// Ceiling division for memory allocation
				uint bits_needed = (pos + 1) * rep->base_bits[k];
				uint words_needed = (bits_needed + W - 1) / W;
				uint bits_current = pos * rep->base_bits[k];
				uint words_current = (bits_current + W - 1) / W;

				if (words_needed > words_current) {
					rep->levels[k] = (uint*)realloc(rep->levels[k], sizeof(uint) * words_needed);
					// Initialize new words to 0
					memset(&rep->levels[k][words_current], 0, (words_needed - words_current) * sizeof(uint));
				}

				bitwrite(rep->levels[k], pos * rep->base_bits[k], rep->base_bits[k], newvalue % rep->base[k]);
				newvalue = newvalue / rep->base[k];

				appendRank(rep->bS[k], 0);
			}

			// Write to termination level j
			k = j;
			if (j < rep->nLevels - 1) {
				pos = rep->bS[j]->n;
			}
			else {
				pos = rep->sizeLastLevel;
			}

			// Ceiling division for memory allocation
			uint bits_needed = (pos + 1) * rep->base_bits[j];
			uint words_needed = (bits_needed + W - 1) / W;
			uint bits_current = pos * rep->base_bits[j];
			uint words_current = (bits_current + W - 1) / W;

			if (words_needed > words_current) {
				rep->levels[j] = (uint*)realloc(rep->levels[j], sizeof(uint) * words_needed);
				for (uint w = words_current; w < words_needed; w++) {
					rep->levels[j][w] = 0;
				}
			}

			bitwrite(rep->levels[j], pos * rep->base_bits[j], rep->base_bits[j], newvalue % rep->base[j]);

			if (j < rep->nLevels - 1) {
				appendRank(rep->bS[k], 1);
			}
			else {
				rep->sizeLastLevel++;
			}
			break;
		}
		j--;
	}

	rep->listLength++;
}

void insertFT(FTRep* rep, uint position, uint value) {
	// If inserting at end, just use appendFT
	if (position == rep->listLength + 1) {
		appendFT(rep, value);
		return;
	}

	uint imax, rest = 0;

	// Calculate which level this value needs
	uint threshold_at_imax = 0;
	{
		imax = 0;
		uint threshold = 0;
		uint capacity = rep->base[0];

		for (uint j = 1; j <= rep->nLevels; j++) {
			threshold += capacity;
			if (value >= threshold) {
				imax = j;
				threshold_at_imax = threshold;
			}
			if (j < rep->nLevels) {
				uint old_capacity = capacity;
				capacity *= rep->base[j];
				if (capacity == 0 || capacity < old_capacity) {
					break;
				}
			}
		}
	}

	// Calculate rest if we need a new level
	if (imax >= rep->nLevels) {
		uint bits_needed = value > 0 ? bits(value) : 1;
		uint bits_covered = 0;
		for (uint i = 0; i < rep->nLevels; i++) {
			bits_covered += rep->base_bits[i];
		}
		rest = bits_needed > bits_covered ? (bits_needed - bits_covered) : 1;
	}

	// Create new level if needed
	uint* auxLL = NULL;
	uint old_nlev = rep->nLevels;
	uint prevLL = 0;

	if (imax >= rep->nLevels) {
		uint old_sizeLastLevel = rep->sizeLastLevel;
		prevLL = old_sizeLastLevel;

		rep->nLevels++;
		rep->base_bits = (ushort*)realloc(rep->base_bits, sizeof(ushort) * rep->nLevels);
		rep->base = (uint*)realloc(rep->base, sizeof(uint) * rep->nLevels);
		rep->base_bits[old_nlev] = rest;
		rep->base[old_nlev] = 1 << rest;

		rep->bS = (bitRankW32Int**)realloc(rep->bS, sizeof(bitRankW32Int*) * (rep->nLevels - 1));

		// Create bitmap. All existing elements terminate at old last level (bit=1)
		// The new element will get bit=0 if it continues, bit=1 if it terminates
		uint words_for_bitmap = (prevLL + 1 + W - 1) / W;
		if (words_for_bitmap == 0) words_for_bitmap = 1;

		auxLL = (uint*)malloc(sizeof(uint) * words_for_bitmap);

		// Initialize all bits to 1 (existing elements all terminate before new level)
		for (uint ii = 0; ii < words_for_bitmap; ii++) {
			auxLL[ii] = UINT_MAX;
		}

		rep->sizeLastLevel = 0;
		rep->levels = (uint**)realloc(rep->levels, sizeof(uint*) * rep->nLevels);

		uint initial_words = (rest + W - 1) / W;
		if (initial_words == 0) initial_words = 1;
		rep->levels[old_nlev] = (uint*)calloc(initial_words, sizeof(uint));

		// Recalculate imax with new structure
		imax = 0;
		threshold_at_imax = 0;
		uint threshold = 0;
		uint capacity = rep->base[0];
		for (uint j = 1; j <= rep->nLevels; j++) {
			threshold += capacity;
			if (value >= threshold) {
				imax = j;
				threshold_at_imax = threshold;
			}
			if (j < rep->nLevels) {
				uint old_capacity = capacity;
				capacity *= rep->base[j];
				if (capacity == 0 || capacity < old_capacity) {
					break;
				}
			}
		}
	}

	// Insert the value level by level
	uint newvalue = value - threshold_at_imax;
	uint pos = position - 1;  // Convert to 0-based index

	// Insert into all levels before termination level
	for (uint k = 0; k < imax; k++) {
		// Recalculate position for this level using rank
		if (k > 0) {
			pos = pos - rank(rep->bS[k - 1], pos > 0 ? pos - 1 : 0);
		}

		// Calculate current level size in bits
		uint levelsize_bits;
		uint n_elements;

		if (k < rep->nLevels - 1) {
			if (k == old_nlev - 1 && auxLL != NULL) {
				n_elements = prevLL;
			}
			else {
				n_elements = rep->bS[k]->n;
			}
		}
		else {
			n_elements = rep->sizeLastLevel;
		}

		levelsize_bits = n_elements * rep->base_bits[k];

		// Allocate more space if needed for insertion
		uint bits_after = levelsize_bits + rep->base_bits[k];
		uint words_after = (bits_after + W - 1) / W;
		uint words_before = (levelsize_bits + W - 1) / W;

		if (words_after > words_before) {
			rep->levels[k] = (uint*)realloc(rep->levels[k], sizeof(uint) * words_after);
			memset(&rep->levels[k][words_before], 0, (words_after - words_before) * sizeof(uint));
		}

		// Insert space for new bits
		insertBits(rep->levels[k], words_after, pos * rep->base_bits[k], rep->base_bits[k]);

		// Write the chunk
		bitwrite(rep->levels[k], pos * rep->base_bits[k], rep->base_bits[k],
			newvalue % rep->base[k]);
		newvalue = newvalue / rep->base[k];

		// Update bitmap
		if (k == old_nlev - 1 && auxLL != NULL) {
			// Special case: creating bitmap for newly promoted level
			// Clear bit at insertion position (new element continues to next level)
			auxLL[pos / W] &= ~(1U << (pos % W));
			rep->bS[k] = createBitRankW32Int(auxLL, prevLL + 1, 1, FACT_RANK);
			auxLL = NULL;  // Ownership transferred to bS
		}
		else {
			// Insert 0 in bitmap (continues to next level)
			insertRank(rep->bS[k], pos, 0);
		}
	}

	// Insert into termination level
	uint k = imax;

	// Recalculate position for termination level
	if (k > 0) {
		if (k < rep->nLevels - 1 || auxLL == NULL) {
			pos = pos - rank(rep->bS[k - 1], pos > 0 ? pos - 1 : 0);
		}
		// else: if k == old_nlev-1, pos was already updated when we created the bitmap
	}

	uint levelsize_bits;
	uint n_elements;

	if (k < rep->nLevels - 1) {
		n_elements = rep->bS[k]->n;
	}
	else {
		n_elements = rep->sizeLastLevel;
	}

	levelsize_bits = n_elements * rep->base_bits[k];

	uint bits_after = levelsize_bits + rep->base_bits[k];
	uint words_after = (bits_after + W - 1) / W;
	uint words_before = (levelsize_bits + W - 1) / W;

	if (words_after > words_before) {
		rep->levels[k] = (uint*)realloc(rep->levels[k], sizeof(uint) * words_after);
		memset(&rep->levels[k][words_before], 0, (words_after - words_before) * sizeof(uint));
	}

	insertBits(rep->levels[k], words_after, pos * rep->base_bits[k], rep->base_bits[k]);
	bitwrite(rep->levels[k], pos * rep->base_bits[k], rep->base_bits[k],
		newvalue % rep->base[k]);

	if (k < rep->nLevels - 1) {
		// Insert 1 in bitmap (terminates at this level)
		insertRank(rep->bS[k], pos, 1);
	}
	else {
		rep->sizeLastLevel++;
	}

	rep->listLength++;
}

void deleteFT(FTRep* rep, uint param) {
	if (param == 0 || param > rep->listLength) {
		fprintf(stderr, "Invalid delete position %u (valid: 1-%u)\n",
			param, rep->listLength);
		return;
	}

	uint pos = param - 1;  // Convert to 0-based
	uint k = 0;

	while (k < rep->nLevels) {
		// Recalculate position at this level
		if (k > 0) {
			uint rank_before = (pos > 0) ? rank(rep->bS[k - 1], pos - 1) : 0;
			pos = pos - rank_before;
		}

		// Calculate level size in bits
		uint levelsize_bits;
		if (k < rep->nLevels - 1) {
			levelsize_bits = rep->bS[k]->n * rep->base_bits[k];
		}
		else {
			levelsize_bits = rep->sizeLastLevel * rep->base_bits[k];
		}

		// Calculate word counts - FIXED: Use ceiling division consistently
		uint old_words = (levelsize_bits + W - 1) / W;
		uint new_levelsize = levelsize_bits - rep->base_bits[k];
		uint new_words = (new_levelsize + W - 1) / W;

		removeBits(rep->levels[k], old_words,
			pos * rep->base_bits[k], rep->base_bits[k]);

		// Reallocate if needed
		if (new_words < old_words && new_words > 0) {
			rep->levels[k] = (uint*)realloc(rep->levels[k], sizeof(uint) * new_words);
		}

		// Handle last level
		if (k == rep->nLevels - 1) {
			if (rep->sizeLastLevel > 1) {
				rep->sizeLastLevel--;
				break;
			}
			else {
				// Last level becomes empty, so we remove
				if (k == 0) {
					rep->sizeLastLevel = 0;
					break;
				}

				// Save the size of the new last level
				uint new_last_size = rep->bS[k - 1]->n;

				// Destroy the bitmap that's no longer needed
				destroyBitRankW32Int(rep->bS[k - 1]);

				// Free the now-empty level
				free(rep->levels[k]);

				// Shrink the arrays
				rep->nLevels--;

				if (rep->nLevels == 1) {
					free(rep->bS);
					rep->bS = NULL;
				}
				else {
					rep->bS = (bitRankW32Int**)realloc(rep->bS,
						sizeof(bitRankW32Int*) * (rep->nLevels - 1));
				}
				rep->levels = (uint**)realloc(rep->levels,
					sizeof(uint*) * rep->nLevels);
				rep->base_bits = (ushort*)realloc(rep->base_bits,
					sizeof(ushort) * rep->nLevels);
				rep->base = (uint*)realloc(rep->base, sizeof(uint) * rep->nLevels);
				rep->sizeLastLevel = new_last_size;			}
			break;
		}

		uint deleted_bit = deleteRank(rep->bS[k], pos);

		if (deleted_bit) {
			break;
		}

		k++;
	}

	rep->listLength--;
}

void replaceFT(FTRep* rep, uint param, uint value) {
	uint pos = param - 1;

	// Calculate which level the new value terminates at
	uint imax = 0;
	uint threshold_at_imax = 0;
	{
		uint threshold = 0;
		uint capacity = rep->base[0];

		for (uint j = 1; j <= rep->nLevels; j++) {
			threshold += capacity;
			if (value >= threshold) {
				imax = j;
				threshold_at_imax = threshold;
			}
			if (j < rep->nLevels) {
				uint old_capacity = capacity;
				capacity *= rep->base[j];
				if (capacity == 0 || capacity < old_capacity) {
					break;
				}
			}
		}
	}

	// Check if we need to create a new level
	uint nlev = rep->nLevels;
	uint prevLL = 0;

	if (imax >= rep->nLevels) {
		// Calculate bits needed for new level
		uint bits_needed = value > 0 ? bits(value) : 1;
		uint bits_covered = 0;
		for (uint i = 0; i < rep->nLevels; i++) {
			bits_covered += rep->base_bits[i];
		}
		uint rest = bits_needed > bits_covered ? (bits_needed - bits_covered) : 1;
		if (rest >= 32) rest = 31;

		prevLL = rep->sizeLastLevel;

		rep->nLevels++;
		rep->base_bits = (ushort*)realloc(rep->base_bits, sizeof(ushort) * rep->nLevels);
		rep->base = (uint*)realloc(rep->base, sizeof(uint) * rep->nLevels);
		rep->base_bits[nlev] = rest;
		rep->base[nlev] = 1 << rest;

		rep->bS = (bitRankW32Int**)realloc(rep->bS,
			sizeof(bitRankW32Int*) * (rep->nLevels - 1));

		// Create bitmap for old last level IMMEDIATELY
		// All existing elements terminate at the old last level, so all bits are 1
		uint words_for_bitmap = (prevLL + W - 1) / W;
		if (words_for_bitmap == 0) words_for_bitmap = 1;

		uint* auxLL = (uint*)malloc(sizeof(uint) * words_for_bitmap);
		for (uint ii = 0; ii < words_for_bitmap; ii++) {
			auxLL[ii] = UINT_MAX;  // All bits = 1 (all existing elements terminate)
		}

		// Create the bitmap structure NOW
		rep->bS[nlev - 1] = createBitRankW32Int(auxLL, prevLL, 1, FACT_RANK);

		rep->sizeLastLevel = 0;
		rep->levels = (uint**)realloc(rep->levels, sizeof(uint*) * rep->nLevels);

		uint initial_words = (rest + W - 1) / W;
		if (initial_words == 0) initial_words = 1;
		rep->levels[nlev] = (uint*)calloc(initial_words, sizeof(uint));

		// Recalculate imax and threshold
		imax = 0;
		threshold_at_imax = 0;
		uint threshold = 0;
		uint capacity = rep->base[0];
		for (uint j = 1; j <= rep->nLevels; j++) {
			threshold += capacity;
			if (value >= threshold) {
				imax = j;
				threshold_at_imax = threshold;
			}
			if (j < rep->nLevels) {
				uint old_capacity = capacity;
				capacity *= rep->base[j];
				if (capacity == 0 || capacity < old_capacity) {
					break;
				}
			}
		}
	}

	// Calculate chunk values for new value
	uint newvalue = value - threshold_at_imax;
	uint k = 0;
	uint e = 0;  // Old value's termination bit

	// Main loop: overwrite chunks level by level
	while (k <= imax && k < rep->nLevels) {
		if (k > 0 && rep->bS != NULL && rep->bS[k - 1] != NULL) {
			pos = pos - rank(rep->bS[k - 1], pos > 0 ? pos - 1 : 0);
		}

		// Overwrite the chunk at this level
		bitwrite(rep->levels[k], pos * rep->base_bits[k],
			rep->base_bits[k], newvalue % rep->base[k]);

		newvalue = newvalue / rep->base[k];

		// If we're at the last level (no bitmap), we're done
		if (k == rep->nLevels - 1) {
			return;
		}

		// Handle newly created level's bitmap
		// The bitmap was already created with all 1s (all elements terminate)
		// Now we need to update the bit for the current element being replaced
		if (k == nlev - 1 && nlev < rep->nLevels) {
			// If the new value continues to the next level, change bit from 1 to 0
			if (k < imax) {
				e = replaceRank(rep->bS[k], pos, 0);  // Change from 1 to 0
			}
			else {
				e = 1;  // It was already 1, stays 1
			}
			if (e) {
				break;
			}
			k++;
			continue;
		}

		// Replace bitmap bit and get old bit value
		if (k < imax) {
			e = replaceRank(rep->bS[k], pos, 0);  // New value continues
		}
		else {
			e = replaceRank(rep->bS[k], pos, 1);  // New value terminates
		}

		if (e) {
			break;  // Old value terminated here
		}
		k++;
	}

	// Handle depth mismatch between old and new values
	if (!e && k > imax) {
		// Old value goes deeper. Delete remaining chunks
		while (k < rep->nLevels) {

			if (k > 0 && rep->bS != NULL && rep->bS[k - 1] != NULL) {
				pos = pos - rank(rep->bS[k - 1], pos > 0 ? pos - 1 : 0);
			}

			uint levelsize_bits;
			if (k < rep->nLevels - 1) {
				levelsize_bits = rep->bS[k]->n * rep->base_bits[k];
			}
			else {
				levelsize_bits = rep->sizeLastLevel * rep->base_bits[k];
			}

			uint old_words = (levelsize_bits + W - 1) / W;
			uint new_levelsize = levelsize_bits - rep->base_bits[k];
			uint new_words = (new_levelsize + W - 1) / W;

			removeBits(rep->levels[k], old_words,
				pos * rep->base_bits[k], rep->base_bits[k]);

			if (new_words < old_words && new_words > 0) {
				rep->levels[k] = (uint*)realloc(rep->levels[k],
					sizeof(uint) * new_words);
			}

			if (k == rep->nLevels - 1) {
				if (rep->sizeLastLevel > 1) {
					rep->sizeLastLevel--;
					break;
				}
				else {
					if (k > 0) {
						uint new_last_size = rep->bS[k - 1]->n;
						destroyBitRankW32Int(rep->bS[k - 1]);
						free(rep->levels[k]);
						rep->nLevels--;

						if (rep->nLevels == 1) {
							free(rep->bS);
							rep->bS = NULL;
						}
						else {
							rep->bS = (bitRankW32Int**)realloc(rep->bS,
								sizeof(bitRankW32Int*) * (rep->nLevels - 1));
						}

						rep->levels = (uint**)realloc(rep->levels,
							sizeof(uint*) * rep->nLevels);
						rep->base_bits = (ushort*)realloc(rep->base_bits,
							sizeof(ushort) * rep->nLevels);
						rep->base = (uint*)realloc(rep->base,
							sizeof(uint) * rep->nLevels);

						rep->sizeLastLevel = new_last_size;
					}
				}
				break;
			}

			uint deleted_bit = deleteRank(rep->bS[k], pos);
			if (deleted_bit) break;
			k++;
		}
		return;
	}

	k++;

	if (e && k <= imax) {
		// Recalculate position for level k before inserting
		if (k > 0 && k <= rep->nLevels - 1 && rep->bS != NULL && rep->bS[k - 1] != NULL) {
			pos = pos - rank(rep->bS[k - 1], pos > 0 ? pos - 1 : 0);
		}

		// Old value was shorter. Insert remaining chunks
		for (uint level = k; level <= imax; level++) {
			if (level > k && rep->bS != NULL && rep->bS[level - 1] != NULL) {
				pos = pos - rank(rep->bS[level - 1], pos > 0 ? pos - 1 : 0);
			}

			uint levelsize_bits;
			uint n_elements;

			if (level < rep->nLevels - 1) {
				n_elements = rep->bS[level]->n;
			}
			else {
				n_elements = rep->sizeLastLevel;
			}

			levelsize_bits = n_elements * rep->base_bits[level];

			uint bits_after = levelsize_bits + rep->base_bits[level];
			uint words_after = (bits_after + W - 1) / W;
			uint words_before = (levelsize_bits + W - 1) / W;

			if (words_after > words_before) {
				rep->levels[level] = (uint*)realloc(rep->levels[level],
					sizeof(uint) * words_after);
				memset(&rep->levels[level][words_before], 0, (words_after - words_before) * sizeof(uint));
			}

			insertBits(rep->levels[level], words_after,
				pos * rep->base_bits[level], rep->base_bits[level]);

			bitwrite(rep->levels[level], pos * rep->base_bits[level],
				rep->base_bits[level], newvalue % rep->base[level]);
			newvalue = newvalue / rep->base[level];

			if (level < imax) {
				insertRank(rep->bS[level], pos, 0);
			}
			else if (level < rep->nLevels - 1) {
				insertRank(rep->bS[level], pos, 1);
			}
			else {
				rep->sizeLastLevel++;
			}
		}
	}
}

void rebuildFT(FTRep* rep) {
	if (rep == NULL || rep->listLength == 0) {
		return;
	}

	// Decompress the entire array
	uint* decompressed = decompressFT(rep, rep->listLength);
	if (decompressed == NULL) {
		fprintf(stderr, "rebuildFT: decompression failed\n");
		return;
	}

	// Step 2: Create a new optimally-encoded FTRep

	FTRep* new_rep = createFT(decompressed, rep->listLength);
	free(decompressed);

	if (new_rep == NULL) {
		fprintf(stderr, "rebuildFT: createFT failed\n");
		return;
	}

	// Step 3: Check if the encoding actually changed
	int same_encoding = 1;
	if (rep->nLevels != new_rep->nLevels) {
		same_encoding = 0;
	}
	else {
		for (uint i = 0; i < rep->nLevels; i++) {
			if (rep->base_bits[i] != new_rep->base_bits[i]) {
				same_encoding = 0;
				break;
			}
		}
	}

	// If encoding is the same, nothing to do
	if (same_encoding) {
		destroyFT(new_rep);
		return;
	}

	// Step 4: Free old structure's internals
	for (uint i = 0; i < rep->nLevels; i++) {
		free(rep->levels[i]);
	}
	free(rep->levels);

	if (rep->nLevels > 1) {
		for (uint i = 0; i < rep->nLevels - 1; i++) {
			destroyBitRankW32Int(rep->bS[i]);
		}
		free(rep->bS);
	}

	free(rep->base);
	free(rep->base_bits);

	// Step 5: Copy new structure's internals to old structure
	rep->nLevels = new_rep->nLevels;
	rep->levels = new_rep->levels;
	rep->bS = new_rep->bS;
	rep->base = new_rep->base;
	rep->base_bits = new_rep->base_bits;
	rep->sizeLastLevel = new_rep->sizeLastLevel;
	// rep->listLength stays the same (no elements added/removed)

	// Step 6: Free the new_rep shell (but not its internals, which we transferred)
	free(new_rep);
}
