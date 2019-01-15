# ["インテルFPGA SDK for OpenCLプログラミング・ガイド"より](https://www.intel.co.jp/content/www/jp/ja/programmable/documentation/mwh1391807965224.html)

### FPGA-CPU間Bufferのコピー動作についての注意書き
- SoC をターゲットとした OpenCL カーネルの共有メモリーの割り当て
インテルR は、 インテルR SoCs を実行するOpenCL? カーネルが FPGA DDR メモリーに代わって
共有メモリーにアクセスすることを推奨します。 
FPGA DDR メモリーは非常に高い帯域幅でカーネルにアクセス可能です。
しかしながら、ARMR CPU から FPGA DDR メモリーへの読み出し動作と書き込み動作は、
ダイレクト・メモリー・アクセス (DMA) を使用しないため非常に遅いです。

- FPGA DDR メモリーは、テスト目的のためにカーネル間または単一カーネル内で一時データを渡す
場合のみ予約してください。

- ライブラリー関数mallocまたはnew演算子を共有メモリーに物理的に割り当てるために使用することはできません。
また、CL_MEM_USE_HOST_PTRフラグは共有メモリーでは機能しません。

- DDR メモリーでは、共有メモリーは物理的に連続している必要があります。
FPGA は SG-DMA (scatter-gather direct memory access) コントローラー・コアなしでは
仮想的に連続したメモリーを消費することができません。

CPU キャッシュは共有メモリーに対して無効です。

- 共有メモリのプログラミング
```
cl_mem src = clCreateBuffer(…, CL_MEM_ALLOC_HOST_PTR, size, …);
int *src_ptr  = (int*)clEnqueueMapBuffer  (…, src, size, …);
*src_ptr = input_value; //host writes to ptr directly
```

- CreateBufferした領域をMapBufferして、CPUで書き込め  

```
clSetKernelArg (…, src);
clEnqueueNDRangeKernel(…);
clFinish();
printf (“Result = %d\n”, *dst_ptr); //result is available immediately
clEnqueueUnmapMemObject(…, src, src_ptr, …);

```

カーネルが終わったら、データをCPUで読み出せ！
後々動きが遅くなるぞ、UnmapBufer、Releaseしろ！
