#define PP_NARG(...)  PP_NARG_(DUMMY, ##__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define KV(...) PP_NARG(__VA_ARGS__) KV2(PP_NARG(__VA_ARGS__), ##__VA_ARGS__)
#define KV2(N,...) KV3(N, ##__VA_ARGS__)
#define KV3(N,...) KV_ ## N (__VA_ARGS__)
#define  KV_0()
#define  KV_1(K1)                                              ,#K1
#define  KV_2(K1,V1)                                           ,#K1,V1
#define  KV_3(K1,V1,K2)                                        ,#K1,V1,#K2
#define  KV_4(K1,V1,K2,V2)                                     ,#K1,V1,#K2,V2
#define  KV_5(K1,V1,K2,V2,K3)                                  ,#K1,V1,#K2,V2,#K3
#define  KV_6(K1,V1,K2,V2,K3,V3)                               ,#K1,V1,#K2,V2,#K3,V3
#define  KV_7(K1,V1,K2,V2,K3,V3,K4)                            ,#K1,V1,#K2,V2,#K3,V3,#K4
#define  KV_8(K1,V1,K2,V2,K3,V3,K4,V4)                         ,#K1,V1,#K2,V2,#K3,V3,#K4,V4
#define  KV_9(K1,V1,K2,V2,K3,V3,K4,V4,K5)                      ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5
#define KV_10(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5)                   ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5
#define KV_11(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5,K6)                ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5,#K6
#define KV_12(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5,K6,V6)             ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5,#K6,V6
#define KV_13(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5,K6,V6,K7)          ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5,#K6,V6,#K7
#define KV_14(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5,K6,V6,K7,V7)       ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5,#K6,V6,#K7,V7
#define KV_15(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5,K6,V6,K7,V7,K8)    ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5,#K6,V6,#K7,V7,#K8
#define KV_16(K1,V1,K2,V2,K3,V3,K4,V4,K5,V5,K6,V6,K7,V7,K8,V8) ,#K1,V1,#K2,V2,#K3,V3,#K4,V4,#K5,V5,#K6,V6,#K7,V7,#K8,V8


struct llist { int a; struct llist* next;};
#define cons(x,y) (struct llist[]){{x,y}}

#define blah(req1,...) do_blah(req1, #req1, KV(__VA_ARGS__))

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(id) id DEFER(EMPTY)()

#define FMT(...)    (FMT2(PP_NARG(__VA_ARGS__), ##__VA_ARGS__))
#define FMT2(N,...) FMT3(N, ##__VA_ARGS__)
#define FMT3(N,...) FMT_ ## N (__VA_ARGS__)
#define FMT_0()            NULL
#define FMT_1(V1)          cons(V1,NULL)
#define FMT_2(V1,V2)       cons(V1,cons(V2,NULL))
#define FMT_3(V1,V2,V3)    cons(V1,cons(V2,cons(V3,NULL)))
#define FMT_4(V1,V2,V3,V4) cons(V1,cons(V2,cons(V3,cons(V4,NULL))))

blah(required);
blah(required, one);
blah(required, one, FMT(fmt_two,still_two,still_two_again));


do_blah(required, "required", 2 ,"one",((struct llist[]){{fmt_two,(struct llist[]){{still_two,(struct llist[]){{still_two_again,NULL}}}}}}));



