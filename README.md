<p align="center">
  <img src="https://github.com/user-attachments/assets/c19f8b34-2079-4dd7-b2e5-cb1afec885a3" width="200"/>
</p>

# Bitcoin Knobs

Bitcoin Knobs is a fork of [Bitcoin Knots](https://bitcoinknots.org) that takes flexibility a step further. Where others decide what is "safe" or "reasonable," we believe in maximum choice. If that means your node refuses to start, your wallet vanishes into the void, or your peers pretend you don't exist, at least the decision was yours.  

---

## What is Bitcoin Knobs?

Bitcoin Knobs connects to the Bitcoin peer-to-peer network to download and fully validate blocks and transactions. It includes a wallet and graphical user interface, which can be optionally built.  

Unlike projects that aim to protect you from yourself, Knobs is built on the principle that users should have full control over configuration, even if that control comes with outcomes best described as "interesting."  

---

## Why?

Because freedom means being able to tune every setting, even the ones nobody sane would touch. Some people call that dangerous. We call it feature-complete.  

---

## Features

- More knobs and toggles than you will ever need  
- Configurations that can make your node sing, stumble, or collapse  
- A wallet and GUI, just like Knots, but without the training wheels  
- Defaults exist, but you are free to ignore them  

---

## Development Process

Development builds on [Bitcoin Core](https://github.com/bitcoin/bitcoin), merged into Knobs with additional options along the way.  

Pull requests considered unsuitable for Core, too unconventional for Knots, or generally frowned upon elsewhere may still find a home here. If merged, contributors are expected to maintain their changes for future releases.  

---

## Testing

Testing is encouraged. Results may vary.  

- **Automated tests**: unit, regression, integration (see `/src/test` and `/test`).  
- **CI**: builds for Linux, macOS, and Windows.  
- **Manual QA**: useful, especially for the more adventurous changes.  

Remember, outcomes are not guaranteed.  

---

## License

MIT. See [COPYING](COPYING) or https://opensource.org/licenses/MIT.  

---

## Translations

Handled via [Bitcoin Core's Transifex](https://explore.transifex.com/bitcoin/bitcoin/). GitHub PRs for translations are not accepted, as they are periodically overwritten.  
