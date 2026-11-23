override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    setContentView(R.layout.activity_webview)

    val mWebView = findViewById<WebView>(R.id.webview)

    val webSettings = mWebView.settings
    webSettings.javaScriptEnabled = true
    mWebView.loadUrl(getString(R.string.website_url))
    mWebView.webViewClient = HelloWebViewClient()
    WebView.setWebContentsDebuggingEnabled(true)

}