#!/usr/bin/env python3
"""
修复 matplotlib 字体配置的工具脚本

此脚本解决常见的 matplotlib 字体问题，尤其是在处理中文和特殊字符时。
"""

import matplotlib
import os
import sys
import shutil

def fix_fonts():
    """尝试修复 matplotlib 字体配置"""
    print("正在修复 matplotlib 字体配置...")
    
    # 尝试清除 matplotlib 缓存
    try:
        matplotlib_cache = matplotlib.get_cachedir()
        if os.path.exists(matplotlib_cache):
            print(f"删除 matplotlib 缓存目录: {matplotlib_cache}")
            shutil.rmtree(matplotlib_cache, ignore_errors=True)
            print("缓存已清除，matplotlib 将重新构建字体缓存")
        else:
            print(f"matplotlib 缓存目录不存在: {matplotlib_cache}")
    except Exception as e:
        print(f"清除缓存时出错: {e}")
        
    # 检查可用字体
    try:
        import matplotlib.font_manager as fm
        fonts = fm.findSystemFonts()
        print(f"找到 {len(fonts)} 个系统字体")
        
        # 检查常用字体是否可用
        sans_serif_fonts = ['Arial', 'DejaVu Sans', 'Liberation Sans', 'FreeSans', 'SimHei', 'Microsoft YaHei']
        available_sans = []
        
        for font in sans_serif_fonts:
            try:
                if any(font.lower() in f.lower() for f in fonts):
                    available_sans.append(font)
            except:
                pass
        
        if available_sans:
            print(f"可用的 Sans-Serif 字体: {', '.join(available_sans)}")
            # 设置字体
            matplotlib.rcParams['font.sans-serif'] = available_sans + ['sans-serif']
            matplotlib.rcParams['axes.unicode_minus'] = False
            print("字体配置已更新")
        else:
            print("警告: 未找到推荐的 Sans-Serif 字体")
            
    except Exception as e:
        print(f"字体检查错误: {e}")
    
    print("字体配置完成")
    return True

if __name__ == "__main__":
    success = fix_fonts()
    sys.exit(0 if success else 1)
