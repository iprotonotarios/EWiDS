#include "average.h"

uint16_t values[AVERAGE_SIZE];
uint32_t sum;
uint16_t used_values;
uint16_t idx;

void average_init(){
	for(int i=0;i<AVERAGE_SIZE;i++){
		values[i]=0;
	}
	sum = 0;
	used_values = 0;
	idx =0;
}

void average_add(uint16_t _val){
	// remove old value from sum
	sum -= (uint32_t)values[idx];
	// add new value to sum
	values[idx] = _val;
	sum += (uint32_t)_val;
	// update idx
	idx = (idx+1) % AVERAGE_SIZE;
	// update used values (needed only when the number of samples are less than 20)
	if (used_values < AVERAGE_SIZE) used_values++;
}

uint16_t average_get(){
	if (used_values==0) 
		return 0;
	else 
		return (uint16_t)(sum/((uint32_t)used_values));
}