---
layout: default
title: Basalt · 从裸机到 hello world 的自研内核
permalink: /
---

<!-- ============ hero band（奶油画布） ============ -->
<section class="hero">
  <div class="wrap">
    <span class="hero-badge"><span class="dot"></span>Milestone M0 · 已达成</span>
    <h1 class="display-xl">从裸机开始，<em>亲手点亮</em>一块屏幕。</h1>
    <p class="hero-sub">Basalt（玄武岩——地壳之下最坚实的那层石头）是一个自研 x86-64 内核的教学研究项目：
    不借助现成内核框架，用 C++17 子集、C 与汇编三种语言逐层搭建，Limine 负责固件初始化，
    内核自身代码从 64 位长模式入口开始，直至在 framebuffer 上画出第一个字符。</p>
    <div class="hero-cta">
      <a class="btn btn-primary" href="{{ '/PROJECT-GUIDE/' | relative_url }}">阅读项目指南</a>
      <a class="btn btn-secondary" href="{{ '/readme/' | relative_url }}">查看 README</a>
      <a class="btn btn-secondary" href="https://github.com/BS-O/Basalt">GitHub 仓库</a>
    </div>
    <div class="hero-meta">
      <span class="badge-pill">x86-64</span>
      <span class="badge-pill">C++17 / C11 / GNU as</span>
      <span class="badge-pill">Limine v12.10.0 · base rev 3</span>
      <span class="badge-pill">WSL2 + QEMU</span>
      <span class="badge-teal"><b>●</b> make check 全绿</span>
    </div>
  </div>
</section>

<!-- ============ dark band：M0 实拍 + 终端输出（暗色产品表面） ============ -->
<section class="band-dark">
  <div class="wrap">
    <h2 class="display-lg">M0：启动，然后说出第一句话</h2>
    <p class="lead">Limine 完成 16 位 → 32 位 → 64 位长模式的全部固件初始化；Basalt 的第一行汇编从长模式入口开始：
    自建栈、收集 framebuffer，交给 C++ 的 kmain —— 随后屏幕被点亮。</p>
    <div class="dark-grid">
      <figure class="mockup" style="margin:0">
        <div class="chrome"><span class="dots"><i></i><i></i><i></i></span> QEMU · SeaBIOS → Limine → basalt.elf</div>
        <img src="{{ 'assets/m0-screendump.png' | relative_url }}" alt="M0 实拍：QEMU framebuffer 输出 hello world">
      </figure>
      <div class="code-window">
        <div class="chrome"><span class="dots"><i></i><i></i><i></i></span> headless 冒烟 · COM1 串口日志</div>
        <div class="body"><span class="c-dim">$ make check</span>
<span class="c-dim">…timeout 15 qemu-system-x86_64 …</span>
<span class="c-ok">CHECK PASSED: hello world on COM1</span>

<span class="c-dim"># 串口日志原始字节</span>
<span class="c-hello">12K</span>hello world

<span class="c-dim"># 1 = 进入 64 位入口桩
# 2 = 自建栈切换完成
# K = kmain（C++）已接管</span></div>
      </div>
    </div>
    <p class="caption" style="margin:14px 0 0; color:var(--on-dark-soft)">左：1024×768 framebuffer 主显示（Terminus 8×16 字形）。右：CI 可断言的串口通道（ADR-003 双 sink 设计）。</p>
  </div>
</section>

<!-- ============ cream band：特性三卡 ============ -->
<section class="band">
  <div class="wrap">
    <h2 class="display-lg">为什么值得读、值得写</h2>
    <p class="lead">M0 严格限定"能启动 + hello world"，但工程方法与扩展接口按真实内核的标准预留。</p>
    <div class="feature-grid">
      <div class="feature-card">
        <span class="num">01</span>
        <h3>三语言严格分层</h3>
        <p>汇编只做入口桩，C 守住硬件边界，C++ 只出现在子系统层。跨语言一律 extern "C"，
        单向依赖，层间边界即 ABI 边界——每一层都能单独讲清楚。</p>
      </div>
      <div class="feature-card">
        <span class="num">02</span>
        <h3>headless 可验证</h3>
        <p>make check 十五秒冒烟断言串口输出；QEMU 监视器 screendump / 寄存器 / 异常日志
        三板斧定位引导故障——全程无需人工盯屏。M0 的三个引导协议坑就是这么抓出来的。</p>
      </div>
      <div class="feature-card">
        <span class="num">03</span>
        <h3>决策全部留痕</h3>
        <p>引导协议为何从 Multiboot2 改为 Limine、字模为何选 Terminus、arm64 为何只占位——
        六条 ADR 逐条记录背景、备选与后果，文档即站点的治理方式。</p>
      </div>
    </div>
  </div>
</section>

<!-- ============ soft band：引导链路 ============ -->
<section class="band band-soft">
  <div class="wrap">
    <h2 class="display-md">一次加电，四步到 hello world</h2>
    <p class="lead">整条链路在模拟器中运行，不写任何真实磁盘或引导区。</p>
    <div class="chain">
      <div class="step"><span class="no">STEP 1</span><h3>固件与引导程序</h3><p>SeaBIOS/UEFI 交给 Limine：装载 ELF、建页表、切长模式、设置 1024×768 图形模式。</p></div>
      <div class="step"><span class="no">STEP 2</span><h3>64 位入口桩</h3><p>entry64.S：串口探针 '1'，切到自建的 16 KiB 内核栈，探针 '2'。</p></div>
      <div class="step"><span class="no">STEP 3</span><h3>C++ 内核入口</h3><p>kmain 校验 Limine 响应，取出 framebuffer 信息，探针 'K'。</p></div>
      <div class="step"><span class="no">STEP 4</span><h3>点亮屏幕</h3><p>console 双 sink：framebuffer 逐位绘制字形 + COM1 输出同一行字，随后 cli;hlt 静默停机。</p></div>
    </div>
  </div>
</section>

<!-- ============ cream band：文档瓷砖 ============ -->
<section class="band" style="padding-top:0">
  <div class="wrap">
    <h2 class="display-md">从这里开始读</h2>
    <p class="lead">四份入口文档，覆盖"是什么 / 怎么跑 / 为什么 / 下一步"。</p>
    <div class="doc-grid">
      <a class="doc-tile" href="{{ '/readme/' | relative_url }}">
        <span class="kind">README</span>
        <h3>README</h3>
        <p>仓库门面：三秒上手、构建与验证命令、目录速览。</p>
        <span class="go">阅读 →</span>
      </a>
      <a class="doc-tile" href="{{ '/PROJECT-GUIDE/' | relative_url }}">
        <span class="kind">GUIDE</span>
        <h3>项目详细指南</h3>
        <p>每一个文件的用途与用法、构建系统逐项解释、排障三板斧、扩展怎么做。</p>
        <span class="go">阅读 →</span>
      </a>
      <a class="doc-tile" href="https://github.com/BS-O/Basalt/blob/main/docs/ARCHITECTURE.md">
        <span class="kind">ARCHITECTURE</span>
        <h3>架构设计</h3>
        <p>分层模型、引导链路、M0 实测踩坑记录（Limine 协议三关）。</p>
        <span class="go">GitHub 阅读 →</span>
      </a>
      <a class="doc-tile" href="https://github.com/BS-O/Basalt/tree/main/docs/adr">
        <span class="kind">DECISIONS</span>
        <h3>决策记录 ADR</h3>
        <p>六个已拍板决策：协议选型、输出通道、字模来源、工具链、arm64 占位、legacy 归档。</p>
        <span class="go">GitHub 阅读 →</span>
      </a>
    </div>
  </div>
</section>

<!-- ============ coral CTA band ============ -->
<section class="wrap" style="padding-bottom:0">
  <div class="cta-coral">
    <h2>把这块石头搬回你的机器上。</h2>
    <p>三条命令：构建 → 跑起来 → 自动验证。一切发生在模拟器里，不动宿主机一根毫毛。</p>
    <div class="hero-cta">
      <a class="btn btn-cream" href="https://github.com/BS-O/Basalt">克隆仓库</a>
      <a class="btn btn-cream" href="{{ '/PROJECT-GUIDE/' | relative_url }}">先读指南</a>
    </div>
  </div>
</section>
