## General
- Removed custom mongo driver sources
- Added dedicated runtime definitions
- Added dedicated files sources with custom methods
- Removed obsolete dedicated handler sources
- Refactored jeeves main values & internal methods
- Updated cerver & cmongo in development Dockerfile
- Added osiris as dependency in dev Dockerfile

## Models
- Updated actions & roles models with new cmongo types
- Refactored user model related definitions
- Refactored job model definitions & methods

## Controllers
- Added more methods in roles controller
- Added base service controller methods
- Added login & register workflows in users controller
- Added jobs HTTP responses in controller sources
- Moved job handlers workflow to controller

## Routes
- Updated users routes handlers with new methods

## Worker
- Updated worker sources with new methods