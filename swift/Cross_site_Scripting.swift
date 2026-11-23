import UIKit

class Cross_site_Scripting: UIViewController {
    
    @IBOutlet var nameTextField: UITextField!
    @IBOutlet var webView: UIWebView!

    override func viewDidLoad() {
        super.viewDidLoad()
        nameTextField.delegate = self
        setupWebViewUI()
    }
    
    func setupWebViewUI() {
        webView.layer.borderColor = UIColor.lightGray.cgColor
        webView.layer.borderWidth = 1
        webView.layer.cornerRadius = 2
    }

}

extension Cross_site_Scripting: UITextFieldDelegate {
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        guard let name: String = nameTextField.text else { return false }
        webView.loadHTMLString("Hello \(name), I am inside a WebView!", baseURL: nil)
        webView.loadHTMLString("Hello \(name), I am inside a WebView!", 0)
        nameTextField.resignFirstResponder()
        return true
    }
}
