package com.example.cpuloader;

import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private WebView webView;

    private static final String TAG = "MainActivity";
    private PowerManager.WakeLock wakeLock;

    static {
        System.loadLibrary("cpuloader");
    }

    private native void startCpuLoad(int cores);
    private native void stopCpuLoad();
    private boolean isRunning = false;
    private int selectedCores = 1;
    private TextView coresText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // 初始化WebView但不加载
        webView = findViewById(R.id.webview);
        WebSettings webSettings = webView.getSettings();
        webSettings.setJavaScriptEnabled(true);
        webView.setWebViewClient(new WebViewClient());


        // 初始化WakeLock
        PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
        wakeLock = powerManager.newWakeLock(
                PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP,
                "CPULoader::WakeLock"
        );

        Button startButton = findViewById(R.id.start_button);
        Button stopButton = findViewById(R.id.stop_button);
        SeekBar coresSeekBar = findViewById(R.id.cores_seekbar);
        coresText = findViewById(R.id.cores_text);

        // 获取CPU核心数
        int maxCores = Runtime.getRuntime().availableProcessors();
        coresSeekBar.setMax(maxCores - 1); // 从1到最大核心数
        coresSeekBar.setProgress(0); // 默认选择1个核心

        coresSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                // 只更新显示，不执行实际操作
                int newCores = progress + 1;
                selectedCores = newCores;
                coresText.setText("CPU核心数: " + selectedCores);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                // 开始滑动时不做任何操作
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // 停止滑动时才执行实际操作
                if (isRunning) {
                    Log.d(TAG, "Changing CPU load to " + selectedCores + " cores");
                    stopCpuLoad();
                    startCpuLoad(selectedCores);
                    Toast.makeText(MainActivity.this,
                            "CPU负载已更新 (" + selectedCores + " 核心)",
                            Toast.LENGTH_SHORT).show();
                }
            }
        });

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!isRunning) {
                    Log.d(TAG, "Starting CPU load with " + selectedCores + " cores");
                    // 获取WakeLock
                    if (!wakeLock.isHeld()) {
                        wakeLock.acquire();
                    }
                    startCpuLoad(selectedCores);
                    isRunning = true;
                    Toast.makeText(MainActivity.this,
                            "CPU负载已启动 (" + selectedCores + " 核心)",
                            Toast.LENGTH_SHORT).show();
                }
            }
        });

        stopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isRunning) {
                    Log.d(TAG, "Stopping CPU load");
                    stopCpuLoad();
                    isRunning = false;
                    // 释放WakeLock
                    if (wakeLock.isHeld()) {
                        wakeLock.release();
                    }
                    Toast.makeText(MainActivity.this, "CPU负载已停止", Toast.LENGTH_SHORT).show();
                }
            }
        });

        // GPU负载控制按钮
        Button startGpuButton = findViewById(R.id.start_gpu_button);
        Button stopGpuButton = findViewById(R.id.stop_gpu_button);
        
        startGpuButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                webView.loadUrl("https://cznull.github.io/vsbm");
                Toast.makeText(MainActivity.this,
                        "GPU负载已启动",
                        Toast.LENGTH_SHORT).show();
            }
        });

        stopGpuButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                webView.loadUrl("about:blank");
                Toast.makeText(MainActivity.this,
                        "GPU负载已停止",
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // 确保在Activity销毁时释放WakeLock
        if (wakeLock != null && wakeLock.isHeld()) {
            wakeLock.release();
        }
    }
}
