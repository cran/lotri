##' @export
print.lotri <- function(x, ...) {
  .tmp <- x
  .lotri <- attr(.tmp, "lotri")
  class(.tmp) <- NULL
  attr(.tmp, "lotri") <- NULL
  print(.tmp)
  .names <- x$.names
  if (length(.names) > 0) {
    cat(paste0("Properties: ", paste(.names, collapse = ", ")), "\n")
  }
  return(invisible(x))
}

##' @export
print.lotriFix <- function(x, ...) {
  if (inherits(x, "matrix")) {
    .tmp <- x
    .dim <- dim(x)
    .cls <- class(.tmp)
    .lotriFix <- attr(.tmp, "lotriFix")
    .lotriUnfix <- attr(.tmp, "lotriUnfix")
    .lotriEst <- attr(.tmp, "lotriEst")
    .lotriLabels <- attr(.tmp, "lotriLabels")
    if (all(.dim == 0L) & !is.null(.lotriEst)) {
      cat("Lotri Estimates (get with `lotriEst()`):\n")
      print(.lotriEst)
      return(invisible(x))
    }
    attr(.tmp, "lotriFix") <- NULL
    attr(.tmp, "lotriEst") <- NULL
    attr(.tmp, "lotriUnfix") <- NULL
    attr(.tmp, "lotriLabels") <- NULL
    .w <- which(.cls == "lotriFix")
    .cls <- .cls[-.w]
    class(.tmp) <- NULL # Note that a matrix doesn't actually have a class
    if (!is.null(.lotriEst)) {
      cat("Lotri Estimates (get with `lotriEst()`):\n")
      print(.lotriEst)
      cat("\nMatrix:\n")
    }
    print(.tmp)
    if (!is.null(.lotriFix)) {
      cat("this matrix has fixed elements\n")
    } else if (!is.null(.lotriUnfix)) {
      cat("this matrix elements that will be unfixed\n")
    }
    if (!is.null(.lotriLabels)) {
      cat("\nThis matrix has diagonal labels:\n")
      print(.lotriLabels)
      cat("\n")
    }
  } else {
    ## lotri or list
    .lotriEst <- attr(x, "lotriEst")
    if (!is.null(.lotriEst)) {
      cat("Lotri Estimates (get with `lotriEst()`):\n")
      print(.lotriEst)
      cat("\n")
    }
    y <- x
    attr(y, "lotriEst") <- NULL
    attr(y, "lotriLabels") <- NULL
    .cls <- class(y)
    .cls <- .cls[.cls != "lotriFix"]
    class(y) <- .cls
    print(y)
  }
  return(invisible(x))
}

##' @export
str.lotri <- function(object, ...) {
  str(object$.list)
}
