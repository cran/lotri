## ----setup, include = FALSE---------------------------------------------------
knitr::opts_chunk$set(
  collapse = TRUE,
  comment = "#>"
)

## -----------------------------------------------------------------------------
mat <- matrix(c(1, 0.5, 0.5, 1),nrow=2,ncol=2,dimnames=list(c("a", "b"), c("a", "b")))

## -----------------------------------------------------------------------------
library(lotri)

mat <- lotri(a+b ~ c(1,
                     0.5, 1))

## -----------------------------------------------------------------------------
mat <- lotri({
    a+b ~ c(1,
            0.5, 1)
    c ~ 1
    d +e ~ c(1,
             0.5, 1)
})

## -----------------------------------------------------------------------------
mat <- matrix(c(1, 0.5, 0, 0, 0,
                0.5, 1, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 0, 0, 1, 0.5,
                0, 0, 0, 0.5, 1),
              nrow=5, ncol=5,
              dimnames= list(c("a", "b", "c", "d", "e"), c("a", "b", "c", "d", "e")))

## -----------------------------------------------------------------------------
library(Matrix)
mat <- matrix(c(1, 0.5, 0.5, 1),nrow=2,ncol=2,dimnames=list(c("a", "b"), c("a", "b")))
mat <- bdiag(list(mat, matrix(1), mat))
## Convert back to standard matrix
mat <- as.matrix(mat)
##
dimnames(mat) <- list(c("a", "b", "c", "d", "e"), c("a", "b", "c", "d", "e"))

## -----------------------------------------------------------------------------
mat <- lotri({
    a+b ~ c(1,
            0.5, 1) | id
    c ~ 1 | occ
    d +e ~ c(1,
             0.5, 1) | id(lower=3, upper=2, omegaIsChol=FALSE)
})

print(mat)

print(mat$lower)
print(mat$upper)
print(mat$omegaIsChol)

