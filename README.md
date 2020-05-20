# Mini https web server

## Introduce
This repo will show you an easy https web server and can view and download local storage file.

## Run
```
$ make init // set own ca private key and ca information
    
// add ca.crt into your system

$ make runserver

// then visit https://b10615043:4433 in your browser
```

### Remote File copy

#### `/`
homepage：provide `server` volume file list in `storage`

#### `/api/file_list/`

show file list of `storage`

#### `/api/download/<filename>`

download file at path `storage/<filename>`