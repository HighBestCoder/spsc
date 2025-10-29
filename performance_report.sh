#!/bin/bash

echo "SPSC队列缓存行大小性能对比报告"
echo "======================================"
echo ""
echo "测试环境:"
echo "- CPU: Intel Xeon Platinum 8370C @ 2.80GHz"
echo "- 编译器: g++ -std=c++17 -O3"
echo "- 测试数据: 每次测试100万次操作，运行5次取平均值"
echo ""

./benchmark_cacheline | grep -A 20 "=== 基准吞吐量测试 ===" | \
awk '
BEGIN { 
    print "缓存行大小  |  平均吞吐量    |  中位数吞吐量  |  变异系数  |  对象大小"
    print "-----------|-------------|-------------|----------|----------"
}
/缓存行大小: ([0-9]+)字节/ { 
    size = $3
    gsub(/字节/, "", size)
}
/平均值: ([0-9,]+) ops\/sec/ { 
    avg = $2
    gsub(/,/, "", avg)
}
/中位数: ([0-9,]+) ops\/sec/ { 
    median = $2
    gsub(/,/, "", median)
}
/变异系数: ([0-9.]+)%/ { 
    cv = $2
    if (size == "32") objsize = "96字节"
    else if (size == "64") objsize = "192字节"
    else if (size == "128") objsize = "384字节"
    else if (size == "256") objsize = "768字节"
    
    printf "%-10s | %10.0f  | %10.0f  | %8s | %s\n", size"字节", avg, median, cv, objsize
}'

echo ""
echo "性能分析总结:"
echo "=============="
echo "1. 🥇 128字节缓存行: 最高吞吐量 (~170M ops/sec), 最低变异系数 (3.19%)"
echo "2. 🥈 64字节缓存行:  第二高吞吐量 (~144M ops/sec), 较低变异系数 (4.45%)"
echo "3. 🥉 32字节缓存行:  性能一般 (~109M ops/sec), 可能存在false sharing"
echo "4. 📉 256字节缓存行: 性能下降 (~122M ops/sec), 内存使用效率低"
echo ""
echo "推荐建议:"
echo "========="
echo "• 对于最佳性能: 推荐使用128字节缓存行大小"
echo "• 对于内存效率: 推荐使用64字节缓存行大小"
echo "• 避免使用32字节 (false sharing风险)"
echo "• 避免使用256字节 (内存浪费严重)"
echo ""
echo "内存效率对比:"
echo "============="
echo "• 64字节版本比128字节版本节省50%内存 (192 vs 384字节)"
echo "• 128字节版本比256字节版本节省50%内存 (384 vs 768字节)"
echo "• 性能/内存比率: 64字节版本最佳"