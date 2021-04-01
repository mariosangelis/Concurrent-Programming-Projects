/* Library in the philosophy of a CCR */
#include <pthread.h>
#include <unistd.h>
#define CCR_DECLARE(label)      pthread_mutex_t mtx_label; \
								pthread_cond_t R_q1_label; \
								pthread_cond_t R_q2_label; \
								pthread_cond_t R_q3_label; \
								int Rn1_label; \
								int Rn2_label; \
								int Rn3_label; \
								int signalRq1_label; \
								int signalRq2_label; \
								int signalRq3_label; \
								int i_label; \
								int flag_label; \
								int already_broadcast_label;
								
							
#define CCR_INIT(label) 		pthread_mutex_init(&mtx_label,NULL); \
								pthread_cond_init(&R_q1_label,NULL); \
								pthread_cond_init(&R_q2_label,NULL); \
								pthread_cond_init(&R_q3_label,NULL); \
								Rn1_label=0; \
								Rn2_label=0; \
								Rn3_label=0; \
								signalRq1_label=0; \
								signalRq2_label=0; \
								signalRq3_label=0; \
								flag_label=0; \
								already_broadcast_label=0; \
								i_label=0; 
								
#define CCR_EXEC(label,cond,body)   pthread_mutex_lock(&mtx_label); \
									if(Rn2_label!=0 || signalRq1_label!=0 || signalRq2_label!=0 || signalRq3_label!=0) { \
										Rn3_label++; \
										pthread_cond_wait(&R_q3_label,&mtx_label); \
										Rn3_label--; \
										signalRq3_label--; \
										if(signalRq3_label==0 && Rn1_label>0) { \
											Rn1_label--; \
											signalRq1_label++; \
											pthread_cond_signal(&R_q1_label); \
										} \
										if(flag_label!=1 || signalRq1_label!=0){ \
											Rn1_label++; \
											pthread_cond_wait(&R_q1_label,&mtx_label); \
											signalRq1_label--; \
										} \
										Rn2_label++; \
										if(Rn1_label>0) { \
											Rn1_label--; \
											signalRq1_label++; \
											pthread_cond_signal(&R_q1_label); \
											pthread_cond_wait(&R_q2_label,&mtx_label); \
											signalRq2_label--; \
										} \
										else { \
											Rn2_label--; \
											if(Rn2_label>0){ \
												signalRq2_label++; \
												pthread_cond_signal(&R_q2_label); \
												pthread_cond_wait(&R_q2_label,&mtx_label); \
												signalRq2_label--; \
											} \
										} \
									} \
									while(cond==0) { /*if region's condition is false,go to the first queue*/\
										Rn1_label++; \
										if(Rn2_label>0) { /*If there are threads inside second queue,wake up one*/\
											Rn2_label--; \
											signalRq2_label++; \
											pthread_cond_signal(&R_q2_label); \
											pthread_cond_wait(&R_q1_label,&mtx_label); \
											signalRq1_label--; \
										} \
										else{ \
											if(Rn3_label!=0 && Rn2_label==0 && signalRq1_label==0 && signalRq2_label==0 && signalRq3_label==0) { \
												already_broadcast_label=1; \
												for(i_label=0;i_label<Rn3_label;i_label++){ \
													signalRq3_label++; \
													pthread_cond_signal(&R_q3_label); \
												} \
												flag_label=Rn3_label; \
												i_label=0; \
											} \
											pthread_cond_wait(&R_q1_label,&mtx_label); \
											signalRq1_label--; \
										} \
										Rn2_label++; \
										if(Rn1_label>0) { \
											Rn1_label--; \
											signalRq1_label++; \
											pthread_cond_signal(&R_q1_label); \
											pthread_cond_wait(&R_q2_label,&mtx_label); \
											signalRq2_label--; \
										} \
										else { \
											Rn2_label--; \
											if(Rn2_label>0){ \
												signalRq2_label++; \
												pthread_cond_signal(&R_q2_label); \
												pthread_cond_wait(&R_q2_label,&mtx_label); \
												signalRq2_label--; \
											} \
										} \
									} \
									\
									body \
									\
									if(Rn1_label>0) { \
										Rn1_label--; \
										signalRq1_label++; \
										pthread_cond_signal(&R_q1_label); \
										pthread_mutex_unlock(&mtx_label); \
									} \
									else if(Rn2_label>0) { \
										Rn2_label--; \
										signalRq2_label++; \
										pthread_cond_signal(&R_q2_label); \
										pthread_mutex_unlock(&mtx_label); \
									} \
									else if(Rn3_label!=0){ \
										if(already_broadcast_label==0){ \
											for(i_label=0;i_label<Rn3_label;i_label++){ \
												signalRq3_label++; \
												pthread_cond_signal(&R_q3_label); \
											} \
										} \
										else{already_broadcast_label=0;} \
										i_label=0; \
										flag_label=Rn3_label; \
										pthread_mutex_unlock(&mtx_label); \
									} \
									else { \
										pthread_mutex_unlock(&mtx_label); \
									} \
									 
									 
