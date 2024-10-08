import subprocess

def run_c_program(program_path, input_file=None):
    command = [program_path]+input_file
    print(command)

    try:
        #print("ssd:"+program_path+" trace: "+input_file[1])
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,bufsize=1, universal_newlines=True)
        while True:
             process.stdin.flush()
             output = process.stdout.readline()
             if output == '' and process.poll() is not None:
                break
             print(output,end="",flush=True)
        if result.returncode == 0:
            print("程序成功执行")
        else:
            print(f"程序执行失败，返回码: {result.returncode}")
            print("trace: "+ input_file[2])
            print("错误输出:\n", result.stderr)
    except Exception as e:
        print(f"发生错误: {e}")
        print("trace: "+ input_file[2])

if __name__ == "__main__":
    c_program_path = "./ssd"  # 替换为你的C语言程序的路径
    # arg = [ #trace_times, trace_name, block.csv
    #      ["30","../new_trace/web2_pre.csv","1"],
    #       ["30","../new_trace/ads_flag.csv","1"],
    #        ["30","../new_trace/hm_1.csv","1"],
    #        ["30","../newlun/2016021719-LUN3.csv","1"],
    #        ["30","../newlun/2016021813-LUN6.csv","1"],
    #        ["30","../newlun/2016021710-LUN3.csv","1"]
    #     ]
    # arg = [
    # ["30","../new_trace/web2_pre.csv","1"],
    # ["60","../new_trace/ads_flag.csv","1"],
    # ["30","../newlun/2016021719-LUN3.csv","1"],
    # ["30","../newlun/2016021813-LUN6.csv","1"],
    # ["30","../newlun/2016021710-LUN3.csv","1"],
    # ["30","../newlun/2016021714-LUN3.csv","1"],
    # ["30","../newlun/2016021708-LUN3.csv","1"],
    # ["30","../newlun/2016021715-LUN3.csv","1"],
    # ["30","../newlun/2016021812-LUN6.csv","1"],
    # ["30","../newlun/2016021711-LUN3.csv","1"],
    # ["30","../newlun/2016021615-LUN6.csv","1"],
    # ["30","../newlun/2016021619-LUN0.csv","1"]
    # ]
    # arg = [
    # ["30","../newlun/2016021812-LUN6.csv","1","15658456"],
    # ["30","../newlun/2016021714-LUN3.csv","1","13729380"],
    # ["30","../newlun/2016021813-LUN6.csv","1","17305216"],
    #  ["30","../newlun/2016021719-LUN3.csv","1","12885302"]
    # ]
    # arg = [
    # ["30","../newlun/2016021812-LUN0.csv","1","19558364"],
    # ["30","../newlun/2016021613-LUN6.csv","1","20903514"],
    # ["30","../newlun/2016021810-LUN6.csv","1","23124808"],
    # ]
    # arg = [
    # ["30","../newlun/2016021712-LUN3.csv","1","17933144"],
    # ["30","../newlun/2016021714-LUN1.csv","1","15782490"],
    # ["30","../newlun/2016021612-LUN2.csv","1","17047312"],
    # ["30","../newlun/2016021812-LUN1.csv","1","22710774"],
    # ["30","../newlun/2016021810-LUN4.csv","1","18728738"],
    # ["30","../newlun/2016021811-LUN3.csv","1","14459642"],
    # ]
    # arg = [
    # ["15","../new_trace/proj_3.csv","1","2973626"],
    # ["15","../new_trace/proj_4.csv","1","27976454"],
    # ["15","../new_trace/src2_1.csv","1","9495064"],
    # ["15","../new_trace/usr_0.csv","1","1291004"]
    # ]
    # arg = [
    # ["30","../newlun/2016021808-LUN0.csv","1","131072","1"],
    # ["30","../newlun/2016021713-LUN3.csv","1","131072","1"],
    # ["30","../newlun/2016021813-LUN6.csv","1","131072","1"],
    # ["30","../newlun/2016021613-LUN6.csv","1","131072","1"],
    # ["30","../newlun/2016021708-LUN3.csv","1","131072","1"],
    # ["30","../newlun/2016021812-LUN6.csv","1","131072","1"],
    # ["30","../newlun/2016021711-LUN3.csv","1","131072","1"],
    # ["30","../newlun/2016021812-LUN0.csv","1","131072","1"],
    # ["30","../newlun/2016021612-LUN4.csv","1","131072","1"],
    # ["30","../newlun/2016021810-LUN2.csv","1","131072","1"],
    # ["15","../new_trace_order/web1_pre.csv","1","131072","1"],
    # ["15","../new_trace_order/web2_pre.csv","1","131072","1"],
    # ["15","../new_trace_order/ads_flag.csv","1","131072","1"],
    # ["15","../new_trace_order/hm_1.csv","1","131072","1"],
    # ["15","../new_trace_order/usr_0.csv","1","131072","4"],
    # ["15","../new_trace_order/proj_3.csv","1","131072","4"],
    # ["15","../new_trace_order/web_1.csv","1","131072","4"],
    # ["15","../new_trace_order/web_2.csv","1","131072","4"],
    # ["15","../new_trace_order/web_0.csv","1","131072","4"],
    # ["15","../ali/112","1","131072","1"],
    # ["15","../ali/121","1","131072","1"],
    # ["15","../ali/188","1","131072","1"],
    # ["15","../ali/206","1","131072","1"],
    # ["15","../ali/373","1","131072","1"],
    # ["15","../ali/735","1","131072","1"]
    # ]
    arg = [
    # ["2","1","../trace2/wjx_26.csv","1"],
    # ["2","1","../trace2/wjx_41.csv","1"],
    # ["2","1","../trace2/wjx_44.csv","1"],
    # ["2","1","../trace2/wjx_49.csv","1"],
    # ["2","1","../trace2/wjx_50.csv","1"],
    # ["2","1","../trace2/wjx_54.csv","1"],
    # ["2","1","../trace2/wjx_55.csv","1"],
    # ["2","1","../trace2/wjx_57.csv","1"],
    # ["2","1","../trace2/wjx_57from0.csv","1"],
    # ["2","1","../trace2/wjx_60.csv","1"],
    # ["2","1","../trace2/1615_0.csv","1"],
    # ["2","1","../trace2/1615_1.csv","1"],
    # ["2","1","../trace2/1615_2.csv","1"],
    # ["2","1","../trace2/1615_3.csv","1"],
    # ["2","1","../trace2/1615_4.csv","1"],
    # ["2","1","../trace2/1615_6.csv","1"],
    # ["2","1","../trace2/1618_3.csv","1"],
    # ["2","1","../trace2/1616_0.csv","1"],
    # ["2","1","../trace2/1616_1.csv","1"],
    # ["2","1","../trace2/1616_3.csv","1"],
    # ["2","1","../trace2/1614_0.csv","1"],  
    # ["2","1","../trace2/1617_0.csv","1"],
    # ["2","1","../trace2/1617_3.csv","1"],
    # ["2","1","../trace2/1616_6.csv","1"],
    # ["2","1","../trace2/ts_0.csv","1"],
    # ["2","1","./trace/mds_0.csv","1"],
    # ["2","1","../trace2/src2_0.csv","1"],
    # ["2","1","../trace2/stg_0.csv","1"],
    # ["2","1","../trace2/wdev_0.csv","1"],
    # ["2","1","../trace2/usr_0.csv","1"],
    # ["2","2","./trace/hm_0.csv","1"],
    # ["2","1","../trace2/82_0_168.csv","1"],
    # ["2","1","../trace2/108_0_168.csv","1"],
    # ["2","1","../trace2/110_0_168.csv","1"],
    # ["2","1","../trace2/119_0_168.csv","1"],
    # ["2","1","../trace2/120_0_168.csv","1"],
    # ["2","1","../trace2/127_0_168.csv","1"],
    # ["2","1","../trace2/137_0_168.csv","1"],
    # ["2","1","../trace2/139_0_168.csv","1"],
    # ["2","1","../trace2/143_0_168.csv","1"],
    # ["5","1","../trace2/148_0_168.csv","1"],
    # ["2","1","../trace2/146_0_168.csv","1"],
    # ["2","1","../trace2/149_0_168.csv","1"],
    # ["2","1","../trace2/152_0_168.csv","1"],
    # ["2","1","../trace2/153_0_168.csv","1"],
    # ["2","1","../trace2/155_0_168.csv","1"],
    # ["2","1","../trace2/156_0_168.csv","1"],
    # ["2","1","../trace2/158_0_168.csv","1"],
    # ["1","1","../trace2/proj_0.csv","1"],
    ["2","1","./trace/myproj_0.csv","1"],
    # ["2","10","./trace/prn_0.csv","1"],
    ["2","1","./trace/myprxy_0.csv","1"],
    # ["2","1","../trace2/src1_2.csv","1"],
    # ["2","0.1","../trace2/142_0_168.csv","1"],
    # ["2","0.1","../trace2/1619_0.csv","1"],
    ]
    
    # arg = [
    # ["15","../ali/112","1","131072","1"],
    # ["15","../ali/121","1","131072","1"],
    # ["15","../ali/188","1","131072","1"],
    # ["15","../ali/206","1","131072","1"],
    # ["15","../ali/373","1","131072","1"],
    # ["15","../ali/735","1","131072","1"]
    # ]
    # arg = [
    #     ["1","../new_trace/ads_flag.csv","1"]
    # ]
    result = subprocess.run(["make","clean"], shell=False, text=True, capture_output=True)
    print("程序输出:\n", result.stdout)
    result = subprocess.run(["make","all"], shell=False, text=True, capture_output=True)
    print("程序输出:\n", result.stdout)
    for i in arg:
        run_c_program(c_program_path,i)
