#!/bin/bash
echo "1. Iniciando controller..."
./bin/controller config.cfg &
sleep 2

echo "2. Iniciando 3 TxGens..."
for i in {1..3}; do
    ./bin/txgen $((i%3+1)) $((500+i*100)) &
done

echo "3. Monitorando por 30 segundos..."
sleep 30
pkill -P $$  # Encerra processos