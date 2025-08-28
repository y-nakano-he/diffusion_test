#!/bin/bash
#!/bin/bash
#SBATCH -p genoa	          #実行パーティション
#SBATCH -N 1		          #ノード数
#SBATCH -J strong_diffusion#  #ジョブ名
#SBATCH -t 02:00:00		     #タイムアウト
#SBATCH --cpus-per-task=192	#最大スレッド数
#SBATCH -o strong-%j.out	     #出力ファイル名

# ====== パラメータ ======
OUT="strong_${SLURM_JOB_ID}.csv"

# CSV ヘッダ
echo "threads,nt,NX,NY,sec,gflops" > "$OUT"

MAX_THREADS=${MAX_THREADS:-${SLURM_CPUS_PER_TASK:-192}}

for ((th=1; th<=MAX_THREADS; th++)); do
     export OMP_NUM_THREADS=$th
     srun --ntasks=1 --cpus-per-task=$th ./diffusion >> "$OUT"
done

echo "Done."
