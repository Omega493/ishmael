# Changelogs

### 25 Oct. 2025
- (Impl.) The logger now runs on a mutex and scoped_lock, preventing race conditions
- (Impl.) A `ConsoleColour` namespace to format the colour of the letters appearing in the console
- (Fix) Removed hardcoded `#include`s